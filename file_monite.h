#pragma once

#include <atomic>
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
        // thread_ = std::make_unique<std::thread>(&FileMonite::do_monite, this);
        thread_ = std::unique_ptr<std::thread>(new std::thread(&FileMonite::do_monite, this));
        return true;
    }

    void stop()
    {
        switch_.store(false, std::memory_order_release);

        if (thread_ && thread_->joinable()) {
            thread_->join();
            thread_.reset();
        }

        // 关闭文件描述符以中断select
        if (inotify_fd_ != -1) {
            close(inotify_fd_);
            inotify_fd_ = -1;
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

        if (inotify_add_watch(inotify_fd_, directory_.c_str(), IN_CLOSE_WRITE) < 0) {
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

            struct timeval timeout = { 1, 0 }; // 1秒超时
            int ret = select(inotify_fd_ + 1, &fdset, nullptr, nullptr, &timeout);
            if (ret < 0) {
                if (errno == EINTR) { // 被信号中断，继续循环
                    continue;
                }
                std::cerr << "select error " << strerror(errno) << std::endl;
                break;
            } else if (ret == 0) { // 超时，继续循环
                continue;
            }

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

                if (event->mask & IN_CLOSE_WRITE && std::string(event->name) == file_ && cb_) {
                    cb_();
                }
            }
        }
    }

private:
    int inotify_fd_ = -1;
    std::atomic<bool> switch_ { false };
    std::string directory_;
    std::string file_;
    std::unique_ptr<std::thread> thread_;
    FileMoniteCallback cb_;
};

}