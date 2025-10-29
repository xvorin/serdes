#include "serdes/example/definition.h"
#include "serdes/serdes.h"

void for_json(int argc, char** argv)
{
    auto ep1 = xvorin::serdes::create<ExampleParameter>();
    ep1->set_sink_file("extest1.json", xvorin::serdes::ParameterSerdesType::PST_JSON);
    ep1->parse_command_line(argc, argv);
    {
        std::unique_lock<std::mutex> guard;
        auto value = ep1->mutable_value(guard);
        value->init_v3();
    }
    ep1->save();

    auto ep2 = xvorin::serdes::create<ExampleParameter>();
    ep2->set_sink_file("extest2.json", xvorin::serdes::ParameterSerdesType::PST_JSON);
    ep2->from_json(ep1->to_json());
    ep2->save();
}

void for_yaml(int argc, char** argv)
{
    auto ep1 = xvorin::serdes::create<ExampleParameter>();
    ep1->set_sink_file("extest1.yaml", xvorin::serdes::ParameterSerdesType::PST_YAML);
    ep1->parse_command_line(argc, argv);
    {
        std::unique_lock<std::mutex> guard;
        auto value = ep1->mutable_value(guard);
        value->init_v3();
    }
    ep1->save();

    auto ep2 = xvorin::serdes::create<ExampleParameter>();
    ep2->set_sink_file("extest2.yaml", xvorin::serdes::ParameterSerdesType::PST_YAML);
    ep2->from_yaml(ep1->to_yaml());
    ep2->save();
}

void for_toml(int argc, char** argv)
{
    auto ep1 = xvorin::serdes::create<ExampleParameter>();
    ep1->set_sink_file("extest1.toml", xvorin::serdes::ParameterSerdesType::PST_TOML);
    ep1->parse_command_line(argc, argv);
    {
        std::unique_lock<std::mutex> guard;
        auto value = ep1->mutable_value(guard);
        value->init_v3();
    }
    ep1->save();

    auto ep2 = xvorin::serdes::create<ExampleParameter>();
    ep2->set_sink_file("extest2.toml", xvorin::serdes::ParameterSerdesType::PST_TOML);
    ep2->from_toml(ep1->to_toml());
    ep2->save();
}

#if ENABLE_PROTOBUF
void for_pbtxt(int argc, char** argv)
{
    auto ep1 = xvorin::serdes::create<ExampleParameter>();

    std::cout << ep1->to_pbdef() << std::endl;

    ep1->set_sink_file("extest1.pbtxt", xvorin::serdes::ParameterSerdesType::PST_PBFMT);
    ep1->parse_command_line(argc, argv);
    {
        std::unique_lock<std::mutex> guard;
        auto value = ep1->mutable_value(guard);
        value->init_v3();
    }
    ep1->save();

    auto ep2 = xvorin::serdes::create<ExampleParameter>();
    ep2->set_sink_file("extest2.pbtxt", xvorin::serdes::ParameterSerdesType::PST_PBFMT);
    ep2->from_pbtxt(ep1->to_pbtxt());
    ep2->save();
}

#include <sys/time.h>
void for_pbbin(size_t sz)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t t1 = tv.tv_sec * 1000000 + tv.tv_usec;

    auto ep1 = xvorin::serdes::create<ExampleParameter>();
    ep1->unsafe_value()->init_v3();
    for (size_t i = 0; i < sz; i++) {
        // ep1->set_package(std::string("pkg") + std::to_string(i));
        // ep1->to_json();
        // ep1->to_yaml();
        // ep1->to_toml();
        ep1->to_pbbin();
    }

    gettimeofday(&tv, NULL);
    int64_t t2 = tv.tv_sec * 1000000 + tv.tv_usec;
    std::cout << "average time cost:" << static_cast<double>(t2 - t1) / sz << "us" << std::endl;
}

#endif

int main(int argc, char** argv)
{
    for_json(argc, argv);
    for_yaml(argc, argv);
    for_toml(argc, argv);
#if ENABLE_PROTOBUF
    for_pbtxt(argc, argv);
    for_pbbin(10000);
#endif

    return 0;
}