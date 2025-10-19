# C++数据序列化与反序列化

以C++数据类型(结构体、STL、基础数据类型)为核心，非侵入式地实现C++数据结构与json/yaml/protobuf/toml/...等序列化格式的互转


# 简单示例
```
#include <serdes/serdes.h>

enum ExampleForEnum {
    E_ONE,
    E_TWO,
    E_THR,
};

struct ExampleBase {
    bool b = true;
};

struct ExampleDerive : public ExampleBase {
    ExampleForEnum e = E_TWO; // 支持enum

    ExampleDerive() = default; // 必须要有默认构造
    ExampleDerive(bool b, ExampleForEnum e) // 示例代码为赋值方便,非必须!
    {
        this->b = b;
        this->e = e;
    }
};

struct ExampleRoot {
    int i = 5; // i的默认值为5,其他成员初值也用相同方式指定
    std::string s = "Hi";
    double d = 3.14;

    // 支持不可打印字符串(buffer)
    // xvorin::serdes::buffer接口完全同std::string
    // xvorin::serdes::buffer类型的数据, protobuf会设定为bytes类型，非protobuf序列化时会进行base64编码
    xvorin::serdes::buffer buffer = std::string("\0\1\2\3\4\5\6\7", 8);

    /// 支持STL(list/vector/map/unordered_map/set/unordered_set),支持STL的任意嵌套使用
    std::vector<std::string> v = { "Hello", "World", "!" };
    std::map<std::string, ExampleDerive> m = { { "A", { true, E_ONE } }, { "B", { false, E_TWO } } };
};

void show(const std::string& key, const std::string content)
{
    std::cout << "***************" << key << "***************" << std::endl
              << content << std::endl;
}

int main(int argc, char** argv)
{
    auto sender = xvorin::serdes::create<ExampleRoot>();

    // 查看数据结构
    show("debug_string(sender)", sender->debug_string());

    // 支持通过结构体直接修改数据
    {
        std::unique_lock<std::mutex> lock;
        auto& value = sender->mutable_value(lock);
        value.i = 64;
        value.d = 456;
        value.v = { "Hello", "My", "Serdes" };
        value.m = { { "B", { true, E_TWO } }, { "C", { false, E_THR } } };
    }

    // 支持通过结构体直接读取数据
    auto value = sender->value();

    // 序列化为各类格式
    show("to_json", sender->to_json());
    show("to_toml", sender->to_toml());
    show("to_yaml", sender->to_yaml());

    // 反序列化
    auto receiver = xvorin::serdes::create<ExampleRoot>("receiver");
    receiver->from_json(sender->to_json());
    receiver->from_toml(sender->to_toml());
    receiver->from_yaml(sender->to_yaml());

#if ENABLE_PROTOBUF
    // 支持Protobuf(3.19.6), 因Protobuf过重, 默认不开启, 可在serdes/serdes/config.h中配置启用
    show("to_pbtxt", receiver->to_pbtxt());
    show("to_pbbin", xvorin::serdes::base64_encode(receiver->to_pbbin()));

    // 生成Protobuf DebugString
    show("to_pbdbstr", receiver->to_pbdbstr(true));
    // 生成proto文件
    show("to_pbdef", receiver->to_pbdef("serdes.example"));
#endif

    show("debug_string(receiver before modify)", receiver->debug_string());

    // 支持通过接口删除数据
    receiver->remove("receiver.v.0");
    receiver->remove("receiver.m.B");

    // 支持通过接口添加/修改数据
    receiver->create("receiver.m.A");
    receiver->set<bool>("receiver.m.A.b", true); // 支持指定类型修改
    receiver->set("receiver.m.A.e", "E_THR"); // 支持不指定类型，通过字符串修改

    show("debug_string(receiver after modify)", receiver->debug_string());

    // 支持通过接口读取数据
    show("receiver.v.1", receiver->get<std::string>("receiver.v.1"));

    // 通过命令行操作数据结构
    receiver->parse_command_line(argc, argv);

    // 将内容保存到文件, 默认使用toml支持保存注释信息
    receiver->set_sink_file("./example.toml");
}

// 以下通过非侵入方式 获取数据结构的信息
DEFINE_ENUM(ExampleForEnum, E_ONE)
DEFINE_ENUM(ExampleForEnum, E_TWO)
DEFINE_ENUM(ExampleForEnum, E_THR)

DEFINE_PARAM(ExampleBase, b, "bool示例") // 字符串"bool示例"为变量的注释信息，可以不写:DEFINE_PARAM(ExampleBase, b)
DEFINE_PARAM(ExampleDerive, e, "enum示例")
DEFINE_INHERIT(ExampleBase, ExampleDerive) // 注册继承关系

DEFINE_PARAM(ExampleRoot, i, "整型值示例")
DEFINE_PARAM(ExampleRoot, s, "字符串示例")
DEFINE_PARAM(ExampleRoot, d, "浮点数示例")
DEFINE_PARAM(ExampleRoot, buffer, "非可打印数据")
DEFINE_PARAM(ExampleRoot, v, "std::vector示例")
DEFINE_PARAM(ExampleRoot, m, "std::map示例")
```

# 示例输出
```
***************debug_string(sender)***************
|root size = 6 [ExampleRoot]
| |root.i = 5 [int] #整型值示例
| |root.s = Hi [std::string] #字符串示例
| |root.d = 3.140000 [double] #浮点数示例
| |root.buffer = AAECAwQFBgc= [xvorin::serdes::buffer] #非可打印数据
| |root.v size = 3 [std::string] #std::vector示例
| | |root.v.0 = Hello [std::string]
| | |root.v.1 = World [std::string]
| | |root.v.2 = ! [std::string]
| |root.m size = 2 [ExampleDerive] #std::map示例
| | |root.m.A size = 2 [ExampleDerive:ExampleBase]
| | | |root.m.A.b = true [bool] #bool示例
| | | |root.m.A.e = E_ONE(0) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
| | |root.m.B size = 2 [ExampleDerive:ExampleBase]
| | | |root.m.B.b = false [bool] #bool示例
| | | |root.m.B.e = E_TWO(1) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
***************to_json***************
{
    "i": 64,
    "s": "Hi",
    "d": 456.0,
    "buffer": "AAECAwQFBgc=",
    "v": [
        "Hello",
        "My",
        "Serdes"
    ],
    "m": {
        "B": {
            "b": true,
            "e": "E_TWO"
        },
        "C": {
            "b": false,
            "e": "E_THR"
        }
    }
}
***************to_toml***************
#整型值示例
i = 64
#字符串示例
s = "Hi"
#浮点数示例
d = 456.0
#非可打印数据
buffer = "AAECAwQFBgc="
v = ["Hello", "My", "Serdes"]

#std::map示例
[m]
[m.B]
#bool示例
b = true
#enum示例
e = "E_TWO"

[m.C]
#bool示例
b = false
#enum示例
e = "E_THR"


***************to_yaml***************
i: 64
s: Hi
d: 456.0
buffer: AAECAwQFBgc=
v:
  - Hello
  - My
  - Serdes
m:
  B:
    b: true
    e: E_TWO
  C:
    b: false
    e: E_THR

***************to_pbtxt***************
i: 64
s: "Hi"
d: 456
buffer: "\000\001\002\003\004\005\006\007"
v: "Hello"
v: "My"
v: "Serdes"
m {
  key: "B"
  value {
    b: true
    e: E_TWO
  }
}
m {
  key: "C"
  value {
    e: E_THR
  }
}

***************to_pbbin***************
CEASAkhpGQAAAAAAgHxAIggAAQIDBAUGByoFSGVsbG8qAk15KgZTZXJkZXMyCQoBQhIECAEQATIHCgFDEgIQAg==
***************to_pbdbstr***************
i: 64 s: "Hi" d: 456 buffer: "\000\001\002\003\004\005\006\007" v: "Hello" v: "My" v: "Serdes" m { key: "B" value { b: true e: E_TWO } } m { key: "C" value { e: E_THR } }
***************to_pbdef***************
syntax = "proto3";
package serdes.example;

enum ExampleForEnum {
    E_ONE = 0;
    E_TWO = 1;
    E_THR = 2;
}

message ExampleDerive {
    bool b = 1;
    ExampleForEnum e = 2;
}

message ExampleRoot {
    int32 i = 1;
    string s = 2;
    double d = 3;
    bytes buffer = 4;
    repeated string v = 5;
    map<string, ExampleDerive> m = 6;
}

***************debug_string(receiver before modify)***************
|receiver size = 6 [ExampleRoot]
| |receiver.i = 64 [int] #整型值示例
| |receiver.s = Hi [std::string] #字符串示例
| |receiver.d = 456.000000 [double] #浮点数示例
| |receiver.buffer = AAECAwQFBgc= [xvorin::serdes::buffer] #非可打印数据
| |receiver.v size = 3 [std::string] #std::vector示例
| | |receiver.v.0 = Hello [std::string]
| | |receiver.v.1 = My [std::string]
| | |receiver.v.2 = Serdes [std::string]
| |receiver.m size = 2 [ExampleDerive] #std::map示例
| | |receiver.m.B size = 2 [ExampleDerive:ExampleBase]
| | | |receiver.m.B.b = true [bool] #bool示例
| | | |receiver.m.B.e = E_TWO(1) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
| | |receiver.m.C size = 2 [ExampleDerive:ExampleBase]
| | | |receiver.m.C.b = false [bool] #bool示例
| | | |receiver.m.C.e = E_THR(2) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
***************debug_string(receiver after modify)***************
|receiver size = 6 [ExampleRoot]
| |receiver.i = 64 [int] #整型值示例
| |receiver.s = Hi [std::string] #字符串示例
| |receiver.d = 456.000000 [double] #浮点数示例
| |receiver.buffer = AAECAwQFBgc= [xvorin::serdes::buffer] #非可打印数据
| |receiver.v size = 2 [std::string] #std::vector示例
| | |receiver.v.0 = My [std::string]
| | |receiver.v.1 = Serdes [std::string]
| |receiver.m size = 2 [ExampleDerive] #std::map示例
| | |receiver.m.A size = 2 [ExampleDerive:ExampleBase]
| | | |receiver.m.A.b = true [bool] #bool示例
| | | |receiver.m.A.e = E_THR(2) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
| | |receiver.m.C size = 2 [ExampleDerive:ExampleBase]
| | | |receiver.m.C.b = false [bool] #bool示例
| | | |receiver.m.C.e = E_THR(2) [ExampleForEnum(E_ONE,E_TWO,E_THR)] #enum示例
***************receiver.v.1***************
Serdes
```




```
1f4e90608d5513f50a5397dd4d4b9fcf  extest1.json
6df7b52431f587b32c00885afd84fcd4  extest1.pbtxt
136d4f02ad270ff2f5d82e981513b1d4  extest1.toml
a6ac16dc2c804352d20c57590795d04d  extest1.yaml
1f4e90608d5513f50a5397dd4d4b9fcf  extest2.json
6df7b52431f587b32c00885afd84fcd4  extest2.pbtxt
136d4f02ad270ff2f5d82e981513b1d4  extest2.toml
a6ac16dc2c804352d20c57590795d04d  extest2.yaml
```
