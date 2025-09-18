#include "definition.h"
#include "serdes/serdes.h"

template <typename T>
bool on_basic_value_change(const std::string& index, T o, T n)
{
    // LOG(INFO) << "  on_basic_value_change: " << index << ", old=" << o << ", new=" << n << std::endl;
    return true;
}

void on_composite_value_change(const std::string& index, xvorin::serdes::ParameterInformType pit)
{
    std::string action;
    if (pit == xvorin::serdes::ParameterInformType::PIT_CREATE) {
        action = "create";
    }
    if (pit == xvorin::serdes::ParameterInformType::PIT_REMOVE) {
        action = "remove";
    }
    if (pit == xvorin::serdes::ParameterInformType::PIT_CHANGE) {
        action = "change";
    }
    // LOG(INFO) << "  on_composite_value_change: " << index << " " << action << std::endl;
}

DEFINE_PARAM(Param, x, "整型变量", on_basic_value_change<int>);

struct SubSubTest {
    int e = 9;
    std::string f = "Boy!!";
    std::map<int, std::string> mis { { 1, "A" }, { 2, "B" } };
    std::set<std::string> ss { "a", "b", "c", "b" };
};

struct SubTest {
    int c = 7;
    std::string d = "Boy";
    SubSubTest ss;
    std::vector<int> vi { 6, 7, 9 };
};

struct Test {
    int a = 5;
    std::string b = "Good";
    SubTest s;
};

DEFINE_ROOT(Test, "测试结构体");
DEFINE_PARAM(Test, a);
DEFINE_PARAM(Test, b, "字符串变量");
DEFINE_PARAM(Test, s);

DEFINE_ROOT(SubTest, "测试子结构体");
DEFINE_PARAM(SubTest, c);
DEFINE_PARAM(SubTest, d, "字符串变量");
DEFINE_PARAM(SubTest, ss, "子子结构体");
DEFINE_PARAM(SubTest, vi, "vector<int>");

DEFINE_PARAM(SubSubTest, e, "数字变量");
DEFINE_PARAM(SubSubTest, f);
DEFINE_PARAM(SubSubTest, mis, "这里是map");
DEFINE_PARAM(SubSubTest, ss, "这里是set");

int main(int argc, char** argv)
{
    // HIDDEN_PARAM(Param, h); // 支持隐藏参数, 无必要不写入文件

    // 注意：如果默认参数值不符合设置的校验规则, 则程序退出
    auto ywp = xvorin::serdes::instance<Test>("root");

    ywp->set_sink_file("test.toml");
    ywp->parse_command_line(argc, argv);

    std::cout << ywp->to_toml() << std::endl;

    {
        std::unique_lock<std::mutex> guard;
        auto& value = ywp->mutable_value(guard);

        value.a = 888;
        value.b = "666";
        value.s.c = 999;
        value.s.d = "777";
        value.s.ss.e = 111;
        value.s.ss.f = "222";
        value.s.ss.mis = { { 3, "C" }, { 4, "D" } };
        value.s.ss.ss = { "A", "D" };
        value.s.vi = { 9, 8, 7, 6 };
    }

    auto ywp2 = xvorin::serdes::create<Test>();
    ywp2->from_toml(ywp->to_toml());
    std::cout << ywp2->debug_string() << std::endl;
    // std::cout << ywp2->to_yaml() << std::endl;
    // ywp->set_sink_file("abc.yaml");

    // // 解析命令行参数
    // ywp->parse_command_line(argc, argv);

    // // std::cout << ywp->to_pbdef("v2x") << std::endl;
    // std::cout << ywp->debug_string() << std::endl;
    // // std::cout << ywp->to_pbbin() << std::endl;

    // auto ywp2 = xvorin::serdes::instance<Param>("root2");
    // // ywp2->from_pbbin(ywp->to_pbbin());
    // std::cout << ywp2->debug_string() << std::endl;

    // return 0;

    // // 获取参数结构体,可在业务代码中任意使用
    // Param v = ywp->value();

    // // 增, 只支持对STL容器子元素进行添加; 结构体成员以注册字段为准,不可动态增删
    // {
    //     std::cout << "create:" << std::endl;
    //     ywp->create("root.q.mk3");
    // }

    // std::cout << ywp->value().show() << std::endl;

    // // 删,  只支持对STL容器子元素进行添加; 结构体成员以注册字段为准,不可动态增删
    // {
    //     std::cout << "remove:" << std::endl;
    //     ywp->remove("root.q.mk1");
    // }

    // std::cout << ywp->value().show() << std::endl;

    // // 改
    // {
    //     std::cout << "set x:" << std::endl;
    //     // 改1, 通过索引到叶子节点进行修改,须明确具体数据类型
    //     ywp->set("root.x", 10);

    //     std::cout << "set y:" << std::endl;
    //     ywp->set("root.y", std::string("456-12345678"));

    //     std::cout << "set z:" << std::endl;
    //     ywp->set("root.z", false);

    //     // 改2, 通过json串修改,value应为json字符串,无须明确具体数据类型,支持针对非叶子节点做整体修改
    //     std::cout << "from_json:" << std::endl;
    //     ywp->from_json("root.q.mk2", R"({"v1": 4})");

    //     // 改3, 通过json串修改,value应为json字符串,无须明确具体数据类型,支持针对非叶子节点做整体修改
    //     std::cout << "from_yaml:" << std::endl;
    //     ywp->from_yaml("root.q.mk3", R"(v2: 3.66)");
    // }

    // // 查
    // {
    //     // 查1, 通过索引到叶子节点进行查询,须明确具体数据类型
    //     int x = ywp->get<int>("root.x");
    //     std::string y = ywp->get<std::string>("root.y");
    //     bool z = ywp->get<bool>("root.z");
    //     std::cout << "x=" << x << ", y=" << y << ", z=" << z << std::endl;

    //     // 查2, 通过json串修改,value应为json字符串,无须明确具体数据类型,支持针对非叶子节点做整体修改
    //     auto jvalue = ywp->to_json("root.q.mk2", 0);
    //     std::cout << "jvalue=" << jvalue << std::endl;

    //     // 查3, 通过json串修改,value应为json字符串,无须明确具体数据类型,支持针对非叶子节点做整体修改
    //     auto yvalue = ywp->to_yaml("root.q.mk3");
    //     std::cout << "yvalue=" << yvalue << std::endl;
    // }

    // // 通过结构体修改参数值
    // {
    //     std::unique_lock<std::mutex> guard;
    //     auto& mv = ywp->mutable_value(guard);
    //     mv.p.insert("insert_by_struct");
    // }

    // // 保存文件
    // ywp->save();

    // while (1) {
    //     sleep(1);
    // }
}