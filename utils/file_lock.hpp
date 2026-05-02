#pragma once

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/file.h>
#include <system_error>
#include <unistd.h>

namespace xvorin::serdes {

class FileLock {
public:
    // 锁类型
    enum LockType {
        READ_LOCK = LOCK_SH, // 共享锁，多个进程可同时持有
        WRITE_LOCK = LOCK_EX // 独占锁，同时只有一个进程持有
    };

    // ---------- 构造 / 析构 ----------

    // 根据路径打开文件并加锁
    // @param filepath  锁文件路径（可以就是你要操作的数据文件）
    // @param type      锁类型
    // @param block     true=阻塞等待锁，false=非阻塞（立即失败）
    FileLock(const std::string& filepath, LockType type, bool block = true)
        : fd_(-1)
        , owns_fd_(true)
        , locked_(false)
    {
        fd_ = open(filepath.c_str(), O_RDWR | O_CREAT, 0644);
        if (-1 == fd_) {
            throw std::system_error(errno, std::generic_category(), "FileLock: open failed");
        }

        const int operation = block ? type : type | LOCK_NB;

        if (0 != flock(fd_, operation)) {
            close(fd_);
            throw std::system_error(errno, std::generic_category(), "FileLock: flock failed");
        }
        locked_ = true;
    }

    // 使用已有文件描述符加锁（不负责关闭 fd）
    // @param own_fd   true=本对象负责关闭 fd，false=调用者自己管理
    FileLock(int fd, LockType type, bool block = true, bool own_fd = false)
        : fd_(fd)
        , owns_fd_(own_fd)
        , locked_(false)
    {
        const int operation = block ? type : type | LOCK_NB;
        if (0 != flock(fd_, operation)) {
            if (owns_fd_) {
                close(fd_); // ⬅ 补上这句
            }
            throw std::system_error(errno, std::generic_category(),
                "FileLock: flock on existing fd failed");
        }
        locked_ = true;
    }

    ~FileLock()
    {
        if (locked_) {
            unlock(); // 主动释放锁
        }
        if (owns_fd_ && fd_ != -1) {
            close(fd_); // 关闭文件（同时也会释放锁）
        }
    }

    // ---------- 禁止拷贝 ----------
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    // ---------- 移动语义 ----------
    FileLock(FileLock&& other) noexcept
        : fd_(other.fd_)
        , owns_fd_(other.owns_fd_)
        , locked_(other.locked_)
    {
        other.fd_ = -1;
        other.locked_ = false;
        other.owns_fd_ = false;
    }

    FileLock& operator=(FileLock&& other) noexcept
    {
        if (this != &other) {
            if (locked_) {
                unlock();
            }
            if (owns_fd_ && -1 != fd_) {
                close(fd_);
            }

            fd_ = other.fd_;
            owns_fd_ = other.owns_fd_;
            locked_ = other.locked_;

            other.fd_ = -1;
            other.locked_ = false;
            other.owns_fd_ = false;
        }
        return *this;
    }

    // ---------- 接口 ----------

    // 主动解锁（通常析构会自动调用）
    void unlock()
    {
        if (locked_ && -1 != fd_) {
            if (0 != flock(fd_, LOCK_UN)) {
                // 解锁失败只能记录日志，不宜抛异常（析构中尤其如此）
                // 此处简单忽略，实际项目可加 log
            }
            locked_ = false;
        }
    }

    // 尝试非阻塞加锁（静态方法，不创建对象）
    // 返回 true 表示成功获得锁，false 表示锁被他人持有
    static bool try_lock(int fd, LockType type)
    {
        int ret = flock(fd, type | LOCK_NB);
        if (ret == 0) {
            return true;
        }
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return false;
        }
        throw std::system_error(errno, std::generic_category(),
            "FileLock::try_lock failed");
    }

    // 获取底层文件描述符（只读）
    int fd() const { return fd_; }
    bool is_locked() const { return locked_; }

private:
    int fd_;
    bool owns_fd_;
    bool locked_;
};

}