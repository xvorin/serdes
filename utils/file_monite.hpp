#pragma once

#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace xvorin::serdes {

class FileMonite final {
    using FileMoniteCallback = std::function<void()>;

public:
    FileMonite(const std::string& file, FileMoniteCallback cb)
        : cb_(std::move(cb))
    {
        auto dname = file;
        auto bname = file;
        directory_ = dirname(const_cast<char*>(dname.c_str()));
        file_ = basename(const_cast<char*>(bname.c_str()));

        // 创建退出管道
        if (pipe(pipe_fd_) < 0) {
            std::cerr << "pipe creation failure: " << strerror(errno) << std::endl;
        }
    }

    ~FileMonite()
    {
        stop();
    }

    bool start()
    {
        if (!init_inotify()) {
            return false;
        }

        switch_.store(true, std::memory_order_release);
        thread_ = std::unique_ptr<std::thread>(new std::thread(&FileMonite::do_monite, this));
        return true;
    }

    void stop()
    {
        if (!switch_.exchange(false, std::memory_order_acq_rel)) {
            return; // 已经停止
        }

        // 向管道写入数据以唤醒 select
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
    }

private:
    bool init_inotify()
    {
        inotify_fd_ = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (inotify_fd_ < 0) {
            std::cerr << "inotify_init failure for " << directory_ << " " << strerror(errno) << std::endl;
            return false;
        }

        // 覆盖: 写入关闭 / 移入(如 mv 替换) / 新建 / 删除; 回调按文件名过滤
        const uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE;
        if (inotify_add_watch(inotify_fd_, directory_.c_str(), mask) < 0) {
            std::cerr << "inotify_add_watch failure for " << directory_ << " " << strerror(errno) << std::endl;
            close(inotify_fd_);
            inotify_fd_ = -1;
            return false;
        }

        return true;
    }

    void do_monite()
    {
        fd_set fdset;
        char events[4096];

        while (switch_.load(std::memory_order_acquire)) {
            FD_ZERO(&fdset);
            FD_SET(inotify_fd_, &fdset);
            FD_SET(pipe_fd_[0], &fdset);

            const int max_fd = std::max(inotify_fd_, pipe_fd_[0]);
            struct timeval timeout = { 5, 0 }; // 5秒超时
            int ret = select(max_fd + 1, &fdset, nullptr, nullptr, &timeout);
            if (ret < 0) {
                if (errno == EINTR) { // 被信号中断，继续循环
                    continue;
                }
                std::cerr << "select error " << strerror(errno) << std::endl;
                break;
            } else if (ret == 0) { // 超时，继续循环
                continue;
            }

            // 检查退出管道
            if (FD_ISSET(pipe_fd_[0], &fdset)) {
                char dummy = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
                (void)read(pipe_fd_[0], &dummy, sizeof(dummy));
#pragma GCC diagnostic pop
                continue;
            }

            // 处理 inotify 事件
            if (!FD_ISSET(inotify_fd_, &fdset)) {
                continue;
            }

            ssize_t nbytes = read(inotify_fd_, events, sizeof(events));
            if (nbytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) { // 非阻塞模式下无数据可用
                    continue;
                }
                std::cerr << "read error " << strerror(errno) << std::endl;
                break;
            }

            for (char* ptr = events; ptr < events + nbytes;) {
                auto event = reinterpret_cast<struct inotify_event*>(ptr);
                ptr += sizeof(struct inotify_event) + event->len;

                const uint32_t relevant = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE;
                if ((event->mask & relevant) && std::string(event->name) == file_ && cb_) {
                    // 重载失败(如文件正被写入/被删)不应终止监控线程
                    try {
                        cb_();
                    } catch (const std::exception& e) {
                        std::cerr << "file monitor reload failed: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "file monitor reload failed: unknown exception" << std::endl;
                    }
                }
            }
        }

        // 线程结束前的清理
        cleanup_fds();
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
    }

private:
    int inotify_fd_ = -1;
    int pipe_fd_[2] = { -1, -1 }; // 退出管道 [读端, 写端]
    std::atomic<bool> switch_ { false };
    std::string directory_;
    std::string file_;
    std::unique_ptr<std::thread> thread_;
    FileMoniteCallback cb_;
};

}