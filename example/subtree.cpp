// subtree 功能全面示例
// 场景: 主配置引用多个"相机子配置"文件, 每个子文件独立管理一路相机的参数
// 展示: 树内子树 / 各格式加载 / 内联对象 / 环境变量路径 / 容器(vector/map) / get-set 穿透
//       / 结构体缓存同步 / 路径往返 / save 写回子文件(原子写) / 错误处理 / protobuf 视图

#include "serdes/serdes.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <string.h>

// ---- 子配置: 一路相机的参数 ----
struct CameraCfg {
    std::string name = "camera";
    int width = 1280;
    int height = 720;
    double fps = 30.0;
    std::vector<std::string> rois = { "0,0,100,100" };
    xvorin::serdes::envar model_path = "${HOME}/det.onnx"; // 环境变量展开(默认值须用启动前已存在的变量)
};

// ---- 主配置: 引用多个子配置(单个 + vector + map 容器) ----
struct AppCfg {
    int id = 0;
    xvorin::serdes::subtree<CameraCfg> camera; // 单相机子配置引用
    std::vector<xvorin::serdes::subtree<CameraCfg>> cams; // 多相机(容器)
    std::map<std::string, xvorin::serdes::subtree<CameraCfg>> zone; // 区域(map 容器)
};

void show(const std::string& key, const std::string& content)
{
    std::cout << "***************" << key << "***************" << std::endl
              << content << std::endl;
}

// 写子配置文件, 返回路径
static std::string write_cfg(const std::string& path, const std::string& content)
{
    std::ofstream fout(path);
    fout << content;
    return path;
}

// 读取文件全部内容
static std::string read_file_as_string(const std::string& path)
{
    std::ifstream fin(path);
    std::stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

int main()
{
    using namespace xvorin::serdes;
    const std::string dir = "/tmp/serdes_example";
    if (0 != system(("rm -rf " + dir + " && mkdir -p " + dir).c_str())) {
        std::cerr << "cannot prepare dir " << dir << std::endl;
        return 1;
    }

    // 准备各格式子配置文件
    const std::string cam1 = write_cfg(dir + "/cam1.json",
        R"({"name":"front","width":1920,"height":1080,"fps":25.0,"rois":["0,0,800,600","800,0,1120,600"],"model_path":"${CAMERA_MODEL_DIR}/det.onnx"})");
    const std::string cam2 = write_cfg(dir + "/cam2.json",
        R"({"name":"rear","width":1280,"height":720,"fps":30.0,"rois":["100,100,200,200","0,0,640,360"]})");
    const std::string cam3 = write_cfg(dir + "/cam3.yaml", "name: left\nwidth: 640\nheight: 480\nfps: 15.0\nrois:\n  - \"0,0,320,240\"\n");
    const std::string cam4 = write_cfg(dir + "/cam4.toml", "name = \"right\"\nwidth = 640\nheight = 480\nfps = 15.0\nrois = [\"0,0,320,240\"]\n");
    const std::string cam5 = write_cfg(dir + "/cam5.xml", "<cam><name>top</name><width>1920</width><height>1080</height><fps>20.0</fps><rois><item>0,0,960,540</item></rois></cam>");
    // 注意: 每个子文件应只被一个 subtree 节点引用(save 写回时同文件多引用会互相覆盖, 后遍历者胜)
    const std::string zone1 = write_cfg(dir + "/zone.json", R"({"name":"zoneA","width":3200,"height":1200,"fps":12.5,"rois":["0,0,1600,600"]})");
    setenv("CAMERA_MODEL_DIR", dir.c_str(), 1);

    // ---- 1. 主配置从 JSON 加载: subtree 成员外部格式是路径字符串, 内容自动挂入树 ----
    const std::string main_json = std::string(R"({"id":1,"camera":")") + cam1 + R"(","cams":[")" + cam2
        + R"("],"zone":{"A":")" + zone1 + R"("}})";
    auto app = create<AppCfg>("app");
    app->from_json(main_json);

    show("1. debug_string: 子文件内容以子树挂入整体参数树", app->debug_string());

    // ---- 2. get/set 穿透子树与容器 ----
    show("2. get: 单相机/容器元素", std::string("camera.width      = ") + std::to_string(app->get<int>("app.camera.width")) + "\ncams.0.name       = " + app->get<std::string>("app.cams.0.name") + "\nzone.A.fps        = " + std::to_string(app->get<double>("app.zone.A.fps")));

    app->set("app.camera.width", "2560"); // 修改子树内参数
    app->set("app.cams.0.rois.1", "0,0,1920,1080"); // 修改容器元素子树内的 vector 元素
    show("2. set 后", app->debug_string("app.camera"));

    // ---- 3. 结构体缓存同步: subtree<T> 的成员与参数树双向一致 ----
    auto v = app->value();
    show("3. 结构体缓存同步",
        std::string("value().camera.width              = ") + std::to_string(v.camera.width)
            + "\nvalue().camera._subtree_sink_path_ = " + v.camera._subtree_sink_path_
            + "\nvalue().cams[0].rois              = " + v.cams[0].rois[1]);

    // ---- 4. 外部格式往返: 主文件保持路径字符串, 内容不内联 ----
    show("4. to_json(往返: 仍是路径字符串)", app->to_json());

    // ---- 4b. profile 开关: to_xxx 输出子树内容(内联)而非路径 ----
    app->enable_subtree_inline(true);
    show("4b. to_json(内联模式: 子树内容整体输出)", app->to_json());
    app->enable_subtree_inline(false); // 恢复默认路径模式

    // ---- 5. 环境变量路径与 ${VAR} 展开 ----
    show("5. 子文件内 envar 展开(model_path)",
        static_cast<std::string>(app->get<envar>("app.camera.model_path")));

    // ---- 6. 内联对象: 不读文件, 直接作为子树(path 保持原值) ----
    auto inline_app = create<AppCfg>("ia");
    inline_app->from_json(R"({"id":2,"camera":{"name":"inline","width":640,"height":480,"fps":15.0,"rois":["0,0,100,100"]}})");
    show("6. 内联对象加载", inline_app->debug_string("ia.camera"));

    // ---- 7. 各格式子文件加载: yaml/toml/xml 与 json 行为一致 ----
    auto app_yaml = create<AppCfg>("ay");
    app_yaml->from_yaml(std::string("id: 3\ncamera: " + cam3 + "\n"));
    show("7. yaml 子文件加载", app_yaml->debug_string("ay.camera"));

    auto app_toml = create<AppCfg>("at");
    app_toml->from_toml(std::string("id = 4\ncamera = \"" + cam4 + "\"\n"));
    show("7. toml 子文件加载", app_toml->debug_string("at.camera"));

    auto app_xml = create<AppCfg>("ax");
    app_xml->from_xml(std::string("<root><id>5</id><camera>" + cam5 + "</camera></root>"));
    show("7. xml 子文件加载", app_xml->debug_string("ax.camera"));

    // ---- 8. save() 同步写回子文件: 树内修改持久化到子文件(原子写), 主文件仍是路径 ----
    const std::string sink = dir + "/main.json";
    app->set_sink_file(sink, ParameterSerdesType::PST_JSON);
    app->save();

    std::ifstream fin(cam1);
    std::string cam1_content((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    show("8. save 后主文件(sink)", read_file_as_string(sink)); // 见下方 helper
    show("8. save 后子文件 cam1.json 已同步", cam1_content);

    // ---- 9. 错误处理与交叉混用 ----
    try {
        auto bad = create<AppCfg>("bad");
        bad->from_json(R"({"camera":"/tmp/serdes_example/not_exist.json"})");
    } catch (const BadFile& e) {
        show("9. 子文件缺失", std::string("BadFile: ") + e.what());
    }

    // 9a. 交叉混用: 主文件 toml + 子文件 json(按扩展名推断) -> 正常加载
    auto cross = create<AppCfg>("cx");
    cross->from_toml(std::string("id = 6\ncamera = \"" + cam1 + "\"\n"));
    show("9. 交叉混用(toml 主 + json 子文件)", cross->debug_string("cx.camera"));

    // 9b. 无扩展名子文件: 回退主格式(toml)解析, 内容为 json -> ParseTomlException
    const std::string noext = write_cfg(dir + "/cam_noext", R"({"name":"x","width":1,"height":2,"fps":3.0})");
    try {
        auto mismatch = create<AppCfg>("mm");
        mismatch->from_toml(std::string("camera = \"" + noext + "\"\n"));
    } catch (const ParseTomlException& e) {
        show("9. 无扩展名子文件回退主格式解析失败", std::string("ParseTomlException: ") + e.what());
    }

#if ENABLE_PROTOBUF
    // ---- 10. protobuf 视图: subtree 为 string 字段, 容器为 repeated string / map<string,string> ----
    show("10. to_pbdef", app->to_pbdef());
    show("10. to_pbtxt(携带路径, 内容由对端加载)", app->to_pbtxt());
#endif

    return 0;
}

DEFINE_PARAM(CameraCfg, name, "相机名")
DEFINE_PARAM(CameraCfg, width, "图像宽")
DEFINE_PARAM(CameraCfg, height, "图像高")
DEFINE_PARAM(CameraCfg, fps, "帧率")
DEFINE_PARAM(CameraCfg, rois, "ROI列表")
DEFINE_PARAM(CameraCfg, model_path, "模型路径(环境变量展开)")

DEFINE_PARAM(AppCfg, id, "应用id")
DEFINE_PARAM(AppCfg, camera, "单相机子配置引用")
DEFINE_PARAM(AppCfg, cams, "多相机子配置(容器)")
DEFINE_PARAM(AppCfg, zone, "区域子配置(map)")
