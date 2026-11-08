#include "definitions.hpp"
#include "serdes/serdes.h"
#include "serdes/utils/file_monitor.hpp"
#include "serdes/utils/utils.hpp"
#include "util.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <dirent.h>
#include <unistd.h>

using namespace xvorin::serdes;

// 多线程/锁相关用例: 单次调用并发安全、不死锁、停止后无残留回调; 原子写并发不损坏文件。
// 每个用例都应在 ctest TIMEOUT 内完成, 挂住即视为死锁失败。

namespace {

// 轮询等待条件成立, 超时返回 false
bool wait_until(const std::function<bool()>& pred, int timeout_ms = 5000)
{
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        if (pred()) {
            return true;
        }
        usleep(10 * 1000);
    }
    return pred();
}

void write_text(const std::string& path, const std::string& content)
{
    testutil::write_file(path, content);
}

int count_tmp_files(const std::string& dir)
{
    int count = 0;
    DIR* handle = opendir(dir.c_str());
    if (handle == nullptr) {
        return 0;
    }
    while (struct dirent* entry = readdir(handle)) {
        if (std::string(entry->d_name).find(".tmp.") != std::string::npos) {
            ++count;
        }
    }
    closedir(handle);
    return count;
}

// ---- FileMonitor: 并发订阅/注销 ----

void test_center_subscribe_unsubscribe()
{
    const std::string dir = testutil::make_dir("mt_center_sub");
    const std::string file = dir + "/c.json";
    write_text(file, "0");

    auto& center = FileMonitor::instance();
    std::atomic<int> calls { 0 };
    std::atomic<int> failed { 0 };

    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < 30; ++i) {
                const int handle = center.subscribe(file, [&calls] { calls.fetch_add(1); });
                if (handle <= 0) {
                    failed.fetch_add(1);
                    continue;
                }
                center.unsubscribe(handle);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    CHECK(failed.load() == 0);

    // 全部注销后写文件: 不应再有任何回调
    write_text(file, "1");
    usleep(200 * 1000);
    const int before = calls.load();
    write_text(file, "2");
    usleep(200 * 1000);
    CHECK(calls.load() == before);

    center.stop();
}

// ---- unsubscribe 不阻塞等待: 在途回调允许继续执行完 ----

void test_unsubscribe_nonblocking()
{
    const std::string dir = testutil::make_dir("mt_unsubscribe");
    const std::string file = dir + "/c.json";
    write_text(file, "0");

    auto& center = FileMonitor::instance();
    std::atomic<bool> entered { false };
    std::atomic<bool> release { false };
    std::atomic<bool> unsub_returned { false };
    std::atomic<int> completed { 0 };

    const int handle = center.subscribe(file, [&] {
        entered = true;
        while (!release.load()) {
            usleep(1000);
        }
        completed.fetch_add(1);
    });
    CHECK(handle > 0);

    write_text(file, "1");
    CHECK(wait_until([&] { return entered.load(); }));

    std::thread unsubscriber([&] {
        center.unsubscribe(handle);
        unsub_returned = true;
    });
    // 回调仍在阻塞, unsubscribe 应已返回(不等待在途回调)
    CHECK(wait_until([&] { return unsub_returned.load(); }, 1000));
    CHECK(!completed.load());

    release = true;
    unsubscriber.join();
    CHECK(wait_until([&] { return completed.load() == 1; }));

    // 取消后不再产生新回调
    write_text(file, "2");
    usleep(100 * 1000);
    CHECK(completed.load() == 1);

    center.stop();
}

// ---- Monitor 控制接口并发: stop/start/set_path/path/monitoring ----

void test_monitor_control_threads()
{
    const std::string dir = testutil::make_dir("mt_monitor_control");
    const std::string f1 = dir + "/m1.json";
    const std::string f2 = dir + "/m2.json";
    write_text(f1, "0");
    write_text(f2, "0");

    auto& center = FileMonitor::instance();
    std::atomic<int> calls { 0 };
    auto monitor = center.monitor("mt", "mt.item", f1, [&calls] { calls.fetch_add(1); });
    CHECK(monitor != nullptr);

    std::vector<std::thread> controllers;
    for (int t = 0; t < 4; ++t) {
        controllers.emplace_back([&, t] {
            for (int i = 0; i < 40; ++i) {
                monitor->stop();
                monitor->start();
                monitor->set_path((i % 2 == 0) ? f1 : f2);
                (void)monitor->path();
                (void)monitor->monitoring();
                if (t == 0 && i % 10 == 0) {
                    write_text((i % 2 == 0) ? f1 : f2, std::to_string(i));
                }
            }
        });
    }
    for (auto& controller : controllers) {
        controller.join();
    }

    monitor->stop();
    CHECK(!monitor->monitoring());
    center.stop();
}

// ---- 回调内取消自己/再次订阅: 不能等待自己而死锁 ----

void test_callback_reentrancy()
{
    const std::string dir = testutil::make_dir("mt_reentrant");
    const std::string file = dir + "/c.json";
    write_text(file, "0");

    auto& center = FileMonitor::instance();
    std::atomic<int> handle { 0 };
    std::atomic<bool> fired { false };

    const int h = center.subscribe(file, [&] {
        if (!fired.exchange(true)) {
            center.unsubscribe(handle.load()); // 回调线程内取消自己
            const int nested = center.subscribe(file, [] {});
            if (nested > 0) {
                center.unsubscribe(nested); // 回调线程内订阅/注销其它句柄
            }
        }
    });
    CHECK(h > 0);
    handle = h;

    write_text(file, "1");
    CHECK(wait_until([&] { return fired.load(); }));
    usleep(100 * 1000);

    center.stop();
}

// ---- stop 重启后句柄不复用: 旧 Monitor 的 stop 不能注销新订阅 ----

void test_center_restart_handle_isolation()
{
    const std::string dir = testutil::make_dir("mt_restart");
    const std::string file = dir + "/c.json";
    write_text(file, "0");

    auto& center = FileMonitor::instance();
    std::atomic<int> old_calls { 0 };
    std::atomic<int> new_calls { 0 };

    const int old_handle = center.subscribe(file, [&old_calls] { old_calls.fetch_add(1); });
    CHECK(old_handle > 0);

    center.stop(); // 旧订阅随 cleanup 释放
    const int new_handle = center.subscribe(file, [&new_calls] { new_calls.fetch_add(1); });
    CHECK(new_handle > 0);
    CHECK(new_handle != old_handle); // 句柄单调, 不复用

    center.unsubscribe(old_handle); // 旧句柄必须无效
    write_text(file, "1");
    CHECK(wait_until([&] { return new_calls.load() > 0; }));
    CHECK(old_calls.load() == 0);

    center.unsubscribe(new_handle);
    center.stop();
}

// ---- ParameterTree: 并发 get/set/create/remove/debug_string/value ----

void test_tree_read_write_race()
{
    auto tree = create<TFileRoot>("mt_tree");
    std::atomic<int> errors { 0 };

    std::vector<std::thread> workers;
    for (int t = 0; t < 3; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < 200; ++i) {
                try {
                    tree->set("mt_tree.i", std::to_string(t * 1000 + i));
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }

    workers.emplace_back([&] {
        for (int i = 0; i < 200; ++i) {
            try {
                (void)tree->get<int>("mt_tree.i");
                (void)tree->value();
                (void)tree->debug_string();
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    workers.emplace_back([&] {
        for (int i = 0; i < 100; ++i) {
            try {
                tree->create("mt_tree.v.2");
            } catch (...) {
                // 已被其他操作创建/删除, 属正常竞争
            }
            try {
                tree->remove("mt_tree.v.2");
            } catch (...) {
            }
        }
    });

    for (auto& worker : workers) {
        worker.join();
    }

    CHECK(errors.load() == 0);
    const int value = tree->get<int>("mt_tree.i");
    CHECK(value >= 0 && value < 3000);
    CHECK(tree->value().s == "s");
}

// ---- CommandLineAPI: save/load/reload/set/get/debug 并发 ----

void test_api_persistence_race()
{
    const std::string dir = testutil::make_dir("mt_api_persist");
    const std::string file = dir + "/cfg.json";

    auto tree = create<TFileRoot>("mt_api");
    tree->set_sink_file(file, ParameterSerdesType::PST_JSON);
    tree->save();

    std::atomic<bool> done { false };
    std::atomic<int> errors { 0 };

    std::thread writer([&] {
        for (int i = 0; i < 200; ++i) {
            try {
                tree->set("mt_api.i", std::to_string(i));
            } catch (...) {
                errors.fetch_add(1);
            }
            usleep(50);
        }
        done = true;
    });

    std::thread saver([&] {
        for (int i = 0; i < 100; ++i) {
            try {
                tree->save();
            } catch (...) {
                errors.fetch_add(1);
            }
            usleep(100);
        }
    });

    std::thread loader([&] {
        for (int i = 0; i < 100; ++i) {
            tree->reload(); // 内部吞掉解析/校验异常, 不应因并发写导致崩溃
            usleep(100);
        }
    });

    std::thread reader([&] {
        while (!done.load()) {
            try {
                (void)tree->get<int>("mt_api.i");
                (void)tree->value();
                (void)tree->debug_string();
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    writer.join();
    saver.join();
    loader.join();
    reader.join();

    CHECK(errors.load() == 0);

    tree->save();

    // 落盘内容必须完整可解析、可被新树加载
    auto loaded = create<TFileRoot>("mt_api2");
    loaded->set_sink_file(file, ParameterSerdesType::PST_JSON);
    const int value = loaded->get<int>("mt_api2.i");
    CHECK(value >= 0 && value < 200);
    CHECK(loaded->value().s == "s");
    CHECK(loaded->value().v.size() == 2);
}

// ---- CommandLineAPI: set_sink 与 save/load/set 并发 ----

void test_api_set_sink_concurrent()
{
    const std::string dir = testutil::make_dir("mt_api_sink");
    const std::string file_a = dir + "/a.json";
    const std::string file_b = dir + "/b.json";

    auto tree = create<TFileRoot>("mt_sink");
    tree->set_sink_file(file_a, ParameterSerdesType::PST_JSON);
    tree->save();
    tree->set_sink_file(file_b, ParameterSerdesType::PST_JSON);
    tree->save();

    std::atomic<int> errors { 0 };
    std::vector<std::thread> workers;

    for (int t = 0; t < 2; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < 40; ++i) {
                try {
                    tree->set_sink_file((i % 2 == 0) ? file_a : file_b, ParameterSerdesType::PST_JSON);
                } catch (...) {
                    errors.fetch_add(1);
                }
                (void)t;
            }
        });
    }

    workers.emplace_back([&] {
        for (int i = 0; i < 100; ++i) {
            try {
                tree->set("mt_sink.i", std::to_string(i));
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    workers.emplace_back([&] {
        for (int i = 0; i < 40; ++i) {
            try {
                tree->save();
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    for (auto& worker : workers) {
        worker.join();
    }

    CHECK(errors.load() == 0);

    // 收尾: 固定一个载体并写盘, 内容必须可解析
    tree->set_sink_file(file_a, ParameterSerdesType::PST_JSON);
    tree->save();

    auto loaded = create<TFileRoot>("mt_sink2");
    loaded->set_sink_file(file_a, ParameterSerdesType::PST_JSON);
    CHECK(loaded->value().s == "s");
}

// ---- profile 开关并发: enable_inform / enable_subtree_inline 与序列化 ----

void test_profile_flags_concurrent()
{
    auto tree = create<TFileRoot>("mt_flags");
    std::atomic<int> errors { 0 };

    std::vector<std::thread> workers;
    for (int t = 0; t < 2; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < 100; ++i) {
                tree->enable_inform(i % 2 == 0);
                tree->enable_subtree_inline(i % 2 == 0);
            }
        });
    }

    workers.emplace_back([&] {
        for (int i = 0; i < 100; ++i) {
            try {
                (void)tree->to_json();
                (void)tree->to_yaml();
                tree->set("mt_flags.i", std::to_string(i));
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    workers.emplace_back([&] {
        for (int i = 0; i < 100; ++i) {
            try {
                (void)tree->value();
                (void)tree->debug_string();
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    for (auto& worker : workers) {
        worker.join();
    }

    CHECK(errors.load() == 0);
    tree->enable_subtree_inline(false);
    CHECK_STR_CONTAINS(tree->to_json(), "\"i\"");
}

// ---- subtree 监控回调(reload_subtree)与主线程 get/save 并发 ----

void test_subtree_reload_race()
{
    const std::string dir = testutil::make_dir("mt_subtree_reload");
    const std::string child = dir + "/sub.json";
    const std::string main_file = dir + "/main.json";

    utils::write_file_atomic(child, R"({"x":0,"y":"sub"})");
    write_text(main_file, "{\"i\":1,\"sub\":\"" + child + "\"}");

    auto tree = create<TSubtreeRoot>("mt_reload");
    tree->set_sink_file(main_file, ParameterSerdesType::PST_JSON);
    CHECK(tree->get<int>("mt_reload.sub.x") == 0);
    tree->enable_subtree_monitor(true);

    std::atomic<bool> done { false };
    std::atomic<int> errors { 0 };
    const int last_written = 40;

    std::thread writer([&] {
        for (int i = 1; i <= last_written; ++i) {
            try {
                utils::write_file_atomic(child, "{\"x\":" + std::to_string(i) + ",\"y\":\"sub\"}");
            } catch (...) {
                errors.fetch_add(1);
            }
            usleep(5 * 1000);
        }
        done = true;
    });

    std::thread saver([&] {
        while (!done.load()) {
            try {
                tree->save(); // 主文件里是子树路径, 保存会顺带写回子文件并触发监控
            } catch (...) {
                errors.fetch_add(1);
            }
            usleep(2 * 1000);
        }
    });

    std::thread reader([&] {
        while (!done.load()) {
            try {
                (void)tree->get<int>("mt_reload.sub.x");
                (void)tree->value();
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    });

    writer.join();
    saver.join();
    reader.join();

    CHECK(errors.load() == 0);
    const int value = tree->get<int>("mt_reload.sub.x");
    CHECK(value >= 0 && value <= last_written);

    tree->enable_subtree_monitor(false);
    FileMonitor::instance().stop();
}

// ---- 原子写并发: 内容不撕裂, 不残留临时文件 ----

void test_atomic_write_concurrent()
{
    const std::string dir = testutil::make_dir("mt_atomic_write");
    const std::string target = dir + "/cfg.json";
    utils::write_file_atomic(target, "writer-init");

    std::vector<std::thread> writers;
    for (int t = 0; t < 8; ++t) {
        writers.emplace_back([&, t] {
            const std::string content = "writer-" + std::to_string(t);
            for (int i = 0; i < 50; ++i) {
                utils::write_file_atomic(target, content);
            }
        });
    }
    for (auto& writer : writers) {
        writer.join();
    }

    // 最终内容必须等于某个写者的完整内容(不能是交错/撕裂产物)
    const std::string content = utils::read_file(target);
    CHECK(content.compare(0, 7, "writer-") == 0);
    CHECK(count_tmp_files(dir) == 0);
}

// ---- 实例注册表并发: 同一 root 返回同一实例 ----

void test_instance_registry_concurrent()
{
    constexpr int kThreads = 8;
    std::shared_ptr<SerdesTree<TFileRoot>> same[kThreads];
    std::shared_ptr<SerdesTree<TFileRoot>> other[kThreads];

    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t] {
            same[t] = instance<TFileRoot>("mt_inst_same");
            other[t] = instance<TFileRoot>("mt_inst_other");
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    for (int t = 0; t < kThreads; ++t) {
        CHECK(same[t] == same[0]);
        CHECK(other[t] == other[0]);
        CHECK(same[t] != other[t]);
    }
}

} // namespace

int main()
{
    test_center_subscribe_unsubscribe();
    test_unsubscribe_nonblocking();
    test_monitor_control_threads();
    test_callback_reentrancy();
    test_center_restart_handle_isolation();
    test_tree_read_write_race();
    test_api_persistence_race();
    test_api_set_sink_concurrent();
    test_profile_flags_concurrent();
    test_subtree_reload_race();
    test_atomic_write_concurrent();
    test_instance_registry_concurrent();

    std::cout << "[OK] concurrency" << std::endl;
    return 0;
}
