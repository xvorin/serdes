#include "serdes/serdes.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// 验证构想: 子树容器(map 的 value 是 subtree) + inform 事件, 路径命名与 index 确定性对应。
//
// 回调接口为 (index, pit, node):
//   - PIT_CREATE: 节点已存在, 回调里直接 dynamic_cast<Subtree*> 并赋值 path;
//   - PIT_REMOVE: 节点已移除, node 为 nullptr, 路径由 index 推导直接删除文件。
//
// 关键点: inform 沿父链上抛, 子树子孙的 PIT 事件(PIT_CHANGE / 嵌套容器增删)也会到达本回调,
// 必须用"index 层级"过滤, 只处理 cams 的直接子元素(约定 key 不含 '.'), 否则会把子孙事件误当容器元素处理。
//
// 注意: 回调在数据锁内执行, 只能直接读写节点/文件, 禁止调用任何树 API。

using namespace xvorin::serdes;

struct CamCfg {
    std::string name = "camera";
    int width = 1920;
    int height = 1080;
    std::vector<std::string> rois; // 嵌套容器, 用于验证子孙事件会被过滤
};

struct Cameras {
    std::map<std::string, subtree<CamCfg>> cams;
};

static const std::string kDir = "/tmp/serdes_example_inform";
static const std::string kPrefix = "app.cams."; // 根名固定为 app, 元素是 cams 的直接子层

static std::string file_for(const std::string& index)
{
    const auto dot = index.find_last_of('.');
    return kDir + "/" + index.substr(dot + 1) + ".toml";
}

// 只接受 cams 的直接子元素: app.cams.<key> (key 不含 '.')
static bool is_direct_element(const std::string& index)
{
    return index.compare(0, kPrefix.size(), kPrefix) == 0
        && index.find('.', kPrefix.size()) == std::string::npos;
}

static void on_cams_inform(const std::string& index, ParameterInformType pit, const std::shared_ptr<Parameter>& node)
{
    const auto spit = (pit == ParameterInformType::PIT_CREATE ? "create " : (pit == ParameterInformType::PIT_REMOVE ? "remove " : "change "));
    if (!is_direct_element(index)) {
        // 子树子孙的事件(值变更/嵌套容器增删), 与"容器元素"无关, 忽略
        std::cout << "[inform] ignore " << spit << index << std::endl;
        return;
    }

    if (pit == ParameterInformType::PIT_CREATE) {
        auto* sub = dynamic_cast<Subtree*>(node.get());
        if (sub) {
            sub->path = file_for(index); // 直接分配路径(随后的序列化会同步进 value_)
            std::cout << "[inform] create " << index << " -> " << sub->path << std::endl;
        } else {
            std::cerr << "[inform] create " << index << " but node is not subtree" << std::endl;
        }
    } else if (pit == ParameterInformType::PIT_REMOVE) {
        ::remove(file_for(index).c_str()); // 路径可由 index 推导, 节点已删也无妨
        std::cout << "[inform] remove " << index << " deleted " << file_for(index) << std::endl;
    }
}

DEFINE_PARAM(CamCfg, name, "相机名")
DEFINE_PARAM(CamCfg, width, "图像宽")
DEFINE_PARAM(CamCfg, height, "图像高")
DEFINE_PARAM(CamCfg, rois, "roi")

DEFINE_PARAM(Cameras, cams, "相机集(子树容器)", on_cams_inform)

static bool file_exists(const std::string& path)
{
    std::ifstream fin(path);
    return fin.good();
}

int main()
{
    if (0 != system(("rm -rf " + kDir + " && mkdir -p " + kDir).c_str())) {
        std::cerr << "cannot prepare dir " << kDir << std::endl;
        return 1;
    }

    auto app = create<Cameras>("app");
    app->enable_inform(true);

    std::cout << "== create front ==" << std::endl;
    app->create("app.cams.front");
    std::cout << "== create rear ==" << std::endl;
    app->create("app.cams.rear");

    // 触发子树子孙事件: 应被过滤, 不影响容器元素处理
    std::cout << "== set front.width (descendant change) ==" << std::endl;
    app->set("app.cams.front.width", "1280");
    std::cout << "== create front.rois.0 (descendant create) ==" << std::endl;
    app->create("app.cams.front.rois.0");

    app->set_sink_file(kDir + "/main.json", ParameterSerdesType::PST_JSON);
    app->save();
    std::cout << "== after save ==" << std::endl;
    std::cout << (file_exists(kDir + "/front.toml") ? "  ok   " : "  MISS ") << "front.toml" << std::endl;
    std::cout << (file_exists(kDir + "/rear.toml") ? "  ok   " : "  MISS ") << "rear.toml" << std::endl;

    std::cout << "== remove front ==" << std::endl;
    app->remove("app.cams.front");
    std::cout << "== after remove ==" << std::endl;
    std::cout << (file_exists(kDir + "/front.toml") ? "  STILL EXISTS " : "  deleted       ") << "front.toml" << std::endl;
    std::cout << (file_exists(kDir + "/rear.toml") ? "  ok   " : "  MISS ") << "rear.toml" << std::endl;

    return 0;
}
