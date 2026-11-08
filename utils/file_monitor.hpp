#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <errno.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace xvorin::serdes {

class Monitor;

// 共享文件监听(进程级单例): 一个线程 + 一个 inotify fd, 按目录 watch 多个文件,
// 以"路径订阅"分发变更事件。替代早期"每文件一个线程/一个 fd"的监听实现,
// 供多个 subtree/组件共用同一份监听基础设施。
//
// 用法:
//   auto& center = FileMonitor::instance();
//   int h = center.subscribe(path, [&]{ ... });   // 惰性启动监听线程
//   center.unsubscribe(h);
//
// 说明:
//   - 同一路径可被多个订阅者订阅, 变更时全部收到;
//   - 回调在线程中执行, 应尽快返回; 抛异常不影响其他订阅者;
//   - 订阅者须保证回调捕获对象在 unsubscribe 前存活(与早期的裸 this 回调绑定同理)。
class FileMonitor {
public:
    static FileMonitor& instance()
    {
        static FileMonitor center;
        return center;
    }

    ~FileMonitor()
    {
        stop();
    }

    // 订阅文件变更; path 首次订阅时惰性启动监听线程; 返回句柄(>0), 用于 unsubscribe
    int subscribe(const std::string& path, std::function<void()> cb)
    {
        if (path.empty() || !cb) {
            return -1;
        }

        std::lock_guard<std::mutex> guard(lock_);
        if (inotify_fd_ == -1 && !init_inotify()) {
            return -1;
        }

        // 按目录注册 watch(同目录复用)
        std::string dir, base;
        split_path(path, dir, base);
        ensure_watch_locked(dir);

        const int handle = next_handle_++;
        handle_path_[handle] = path;
        by_path_[path].push_back({ handle, std::move(cb) });

        start_locked(); // 惰性启动
        return handle;
    }

    // 面向调用方的监听句柄: 创建即监听, 句柄持有关联实体/参数/文件与回调;
    // stop()/start()/set_path() 控制, 析构自动取消订阅
    std::shared_ptr<Monitor> monitor(const std::string& entity, const std::string& index,
        const std::string& file_path, std::function<void()> on_change);

    // 取消订阅; 句柄不存在时静默忽略(幂等)
    void unsubscribe(int handle)
    {
        if (handle <= 0) {
            return;
        }

        std::lock_guard<std::mutex> guard(lock_);
        auto iter = handle_path_.find(handle);
        if (iter == handle_path_.end()) {
            return;
        }

        const auto& path = iter->second;
        auto& cbs = by_path_[path];
        cbs.erase(std::remove_if(cbs.begin(), cbs.end(),
                      [handle](const std::pair<int, std::function<void()>>& e) { return e.first == handle; }),
            cbs.end());
        if (cbs.empty()) {
            by_path_.erase(path);
        }
        handle_path_.erase(iter);
    }

    // 显式停止监听线程(进程退出/测试收尾用); 单例析构也会调用
    void stop()
    {
        if (!running_.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        // 唤醒 select 后 join
        if (pipe_fd_[1] != -1) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
            (void)write(pipe_fd_[1], "0", 1);
#pragma GCC diagnostic pop
        }

        if (thread_ && thread_->joinable()) {
            thread_->join();
            thread_.reset();
        }

        cleanup_fds();
    }

    bool started() const
    {
        return running_.load(std::memory_order_acquire);
    }

private:
    FileMonitor() = default;
    FileMonitor(const FileMonitor&) = delete;
    FileMonitor& operator=(const FileMonitor&) = delete;

    static void split_path(const std::string& path, std::string& dir, std::string& base)
    {
        const auto pos = path.find_last_of('/');
        if (pos == std::string::npos) {
            dir = ".";
            base = path;
        } else if (pos == 0) {
            dir = "/";
            base = path.substr(1);
        } else {
            dir = path.substr(0, pos);
            base = path.substr(pos + 1);
        }
    }

    bool init_inotify()
    {
        inotify_fd_ = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (inotify_fd_ < 0) {
            return false;
        }

        if (pipe(pipe_fd_) < 0) {
            close(inotify_fd_);
            inotify_fd_ = -1;
            return false;
        }
        return true;
    }

    void ensure_watch_locked(const std::string& dir)
    {
        for (const auto& w : wd_to_dir_) {
            if (w.second == dir) {
                return;
            }
        }

        const uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE;
        const int wd = inotify_add_watch(inotify_fd_, dir.c_str(), mask);
        if (wd >= 0) {
            wd_to_dir_[wd] = dir;
        }
    }

    void start_locked()
    {
        if (!thread_) {
            running_.store(true, std::memory_order_release);
            thread_ = std::unique_ptr<std::thread>(new std::thread(&FileMonitor::do_monitor, this));
        }
    }

    void do_monitor()
    {
        fd_set fdset;
        char events[4096];

        while (running_.load(std::memory_order_acquire)) {
            FD_ZERO(&fdset);
            FD_SET(inotify_fd_, &fdset);
            FD_SET(pipe_fd_[0], &fdset);

            const int max_fd = std::max(inotify_fd_, pipe_fd_[0]);
            struct timeval timeout = { 5, 0 };
            const int ret = select(max_fd + 1, &fdset, nullptr, nullptr, &timeout);
            if (ret < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            if (ret == 0) {
                continue;
            }

            if (FD_ISSET(pipe_fd_[0], &fdset)) {
                char dummy = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
                (void)read(pipe_fd_[0], &dummy, sizeof(dummy));
#pragma GCC diagnostic pop
                continue;
            }

            if (!FD_ISSET(inotify_fd_, &fdset)) {
                continue;
            }

            const ssize_t nbytes = read(inotify_fd_, events, sizeof(events));
            if (nbytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                break;
            }

            for (char* ptr = events; ptr < events + nbytes;) {
                auto ev = reinterpret_cast<struct inotify_event*>(ptr);
                ptr += sizeof(struct inotify_event) + ev->len;

                const uint32_t relevant = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE;
                if (!(ev->mask & relevant) || ev->len == 0) {
                    continue;
                }

                dispatch(ev->wd, ev->name);
            }
        }
    }

    // 找到 (wd, basename) 对应的完整路径并触发其全部订阅者
    void dispatch(int wd, const char* name)
    {
        std::vector<std::function<void()>> cbs;
        {
            std::lock_guard<std::mutex> guard(lock_);
            const auto witer = wd_to_dir_.find(wd);
            if (witer == wd_to_dir_.end()) {
                return;
            }
            std::string path = witer->second;
            if (path != "/") {
                path += "/";
            }
            path += name;

            const auto iter = by_path_.find(path);
            if (iter == by_path_.end()) {
                return;
            }
            for (const auto& e : iter->second) {
                cbs.push_back(e.second);
            }
        }

        for (auto& cb : cbs) {
            try {
                cb();
            } catch (const std::exception& e) {
                // 单个订阅者异常不影响其他订阅者; 打印留痕
                (void)e;
            }
        }
    }

    void cleanup_fds()
    {
        if (inotify_fd_ != -1) {
            close(inotify_fd_);
            inotify_fd_ = -1;
        }
        if (pipe_fd_[0] != -1) {
            close(pipe_fd_[0]);
            pipe_fd_[0] = -1;
        }
        if (pipe_fd_[1] != -1) {
            close(pipe_fd_[1]);
            pipe_fd_[1] = -1;
        }
        wd_to_dir_.clear();
        by_path_.clear(); // 停止后旧订阅全部失效, 避免重启后旧回调复活
        handle_path_.clear();
    }

private:
    int inotify_fd_ = -1;
    int pipe_fd_[2] = { -1, -1 };
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_ { false };
    std::mutex lock_;
    std::map<int, std::string> wd_to_dir_; // watch descriptor -> directory
    std::unordered_map<std::string, std::vector<std::pair<int, std::function<void()>>>> by_path_; // path -> (handle, cb)
    std::unordered_map<int, std::string> handle_path_; // handle -> path
    int next_handle_ = 1;
};

// 监听句柄: 把"实体 + 参数 + 文件"与回调绑定为一个可独立控制的对象。
// 创建即开始监听; stop() 暂停, start() 恢复, set_path() 切换监听文件, 析构自动取消订阅。
class Monitor : public std::enable_shared_from_this<Monitor> {
public:
    ~Monitor()
    {
        stop();
    }

    // 暂停监听(取消订阅, 保留配置与回调; 之后可 start() 恢复)
    void stop()
    {
        int h = 0;
        {
            std::lock_guard<std::mutex> guard(lock_);
            h = handle_;
            handle_ = -1;
        }
        if (h > 0) {
            center_.unsubscribe(h);
        }
    }

    // 恢复监听(重新订阅当前 path)
    void start()
    {
        std::lock_guard<std::mutex> guard(lock_);
        if (handle_ > 0 || path_.empty()) {
            return;
        }
        handle_ = center_.subscribe(path_, cb_);
    }

    // 切换监听文件: 停止旧文件订阅, 若原先在监听则订阅新文件
    void set_path(const std::string& path)
    {
        std::lock_guard<std::mutex> guard(lock_);
        const bool was_on = (handle_ > 0);
        if (was_on) {
            center_.unsubscribe(handle_);
            handle_ = -1;
        }
        path_ = path;
        if (was_on && !path_.empty()) {
            handle_ = center_.subscribe(path_, cb_);
        }
    }

    bool monitoring() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return handle_ > 0;
    }

    std::string path() const
    {
        std::lock_guard<std::mutex> guard(lock_);
        return path_; // 返回值拷贝: 避免调用方读取时被并发 set_path 改写
    }

    const std::string& entity() const
    {
        return entity_;
    }
    const std::string& index() const
    {
        return index_;
    }

    // 工厂: new 的构造点位于本静态成员函数体内, 可访问私有构造
    // (std::make_shared 的构造发生在 allocator 模板上下文, 即使在此也不具访问权)
    static std::shared_ptr<Monitor> create(FileMonitor& center, std::string entity,
        std::string index, std::string path, std::function<void()> cb)
    {
        return std::shared_ptr<Monitor>(new Monitor(center, std::move(entity),
            std::move(index), std::move(path), std::move(cb)));
    }

private:
    friend class FileMonitor;

    Monitor(FileMonitor& center, std::string entity, std::string index,
        std::string path, std::function<void()> cb)
        : center_(center)
        , entity_(std::move(entity))
        , index_(std::move(index))
        , path_(std::move(path))
        , cb_(std::move(cb))
    {
        if (!path_.empty()) {
            handle_ = center_.subscribe(path_, cb_);
        }
    }

private:
    FileMonitor& center_;
    const std::string entity_; // 关联实体标识(诊断/去重)
    const std::string index_; // 参数索引(诊断)
    std::string path_; // 当前监听文件
    std::function<void()> cb_;
    int handle_ = -1;
    mutable std::mutex lock_; // 保护 handle_/path_ 与中心线程并发
};

inline std::shared_ptr<Monitor> FileMonitor::monitor(const std::string& entity,
    const std::string& index, const std::string& file_path, std::function<void()> on_change)
{
    return Monitor::create(*this, entity, index, file_path, std::move(on_change));
}

}
