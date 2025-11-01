#pragma once

#include "serdes/types/parameter_tree.hpp"

#if ENABLE_JSON
#include "serdes/serdes/json/json_api.hpp"
#endif
#if ENABLE_YAML
#include "serdes/serdes/yaml/yaml_api.hpp"
#endif
#if ENABLE_TOML
#include "serdes/serdes/toml/toml_api.hpp"
#endif
#if ENABLE_PROTOBUF
#include "serdes/serdes/protobuf/protobuf_api.hpp"
#endif

#include "serdes/utils/file_monite.hpp"

namespace xvorin::serdes {

template <typename T>
class CommandLineAPI {
public:
    explicit CommandLineAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    /// @brief 命令行参数解析
    /// @param argc 参数个数
    /// @param argv 参数列表
    void parse_command_line(int argc, char** argv);

    /// @brief 设置参数持久化的配置文件
    /// @param sink 配置文件名称
    /// @param sink_type 配置文件格式
    void set_sink_file(const std::string& sink, ParameterSerdesType sink_type = ParameterSerdesType::PST_TOML);

    /// @brief 是否已经开启文件监控
    /// @return
    bool is_monitoring();

    /// @brief 读取设置的配置文件格式
    /// @return
    ParameterSerdesType sink_type();

    /// @brief 加载配置文件中的内容
    void load();

    /// @brief 保存参数到配置文件
    void save();

    /// @brief 重新加载配置文件
    void reload();

private:
    virtual std::string serialize() = 0;
    virtual void deserialize(const std::string&) = 0;
    bool has_cmd(std::vector<std::pair<std::string, std::string>> cmds, const std::string& target, std::string* param = nullptr);

private:
    ParameterTree<T>& tree_;
    std::string sink_;
    ParameterSerdesType sink_type_;
    std::string sink_content_;
    std::mutex sink_content_lock_;
    std::unique_ptr<FileMonite> monite_;
};

template <typename T>
void CommandLineAPI<T>::parse_command_line(int argc, char** argv)
{
    static const auto help_info = R"(
        [--<index> <value>] 临时修改参数值
        [--create  <index>] 临时添加参数
        [--remove  <index>] 临时删除参数
        [--save]            将--<index>/--create/--remove等临时修改持久化到配置文件
        [--show [index]]    展示指定index的参数结构;index选填,不填时展示整体参数结构
        [--help]            展示此帮助信息
    )";

    // clang-format off
    static std::map<std::string, std::function<void(const std::string&)>> actions = {
        { "help",   [&](const std::string& s) { std::cout << "Usage: " << argv[0] << help_info << std::endl; exit(0); } },
        { "show",   [&](const std::string& s) { std::cout << "From " << sink_ << "\n" << tree_.debug_string(s) << std::endl; exit(0);} },
        { "create", [&](const std::string& s) { tree_.create(s); } },
        { "remove", [&](const std::string& s) { tree_.remove(s); } },
        { "save",   nullptr }
    };
    // clang-format on

    std::vector<std::pair<std::string, std::string>> cmds;
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        std::istringstream iss(std::string(argv[i]));
        std::string token;
        while (std::getline(iss, token, '=')) {
            if (!token.empty()) {
                args.push_back(token);
            }
        }
    }

    for (size_t i = 1; i < args.size(); i++) {
        if (0 != args[i].find("--")) {
            continue;
        }

        const std::string cmd = args[i].substr(2);
        if (actions.find(cmd) != actions.end()) {
            cmds.push_back({ cmd, (i + 1 < args.size() && 0 != args[i + 1].find("--")) ? args[++i] : "" });
            continue;
        }

        if (i + 1 >= args.size()) {
            break;
        }

        cmds.push_back({ cmd, args[++i] });
    }

    if (has_cmd(cmds, "help")) {
        actions["help"](""); // exit
    }

    // create or remove
    for (const auto& cmd : cmds) {
        if (cmd.first == "create" || cmd.first == "remove") {
            actions[cmd.first](cmd.second);
        }
    }

    // modify
    for (const auto& cmd : cmds) {
        if (actions.find(cmd.first) == actions.end()) {
            tree_.set(cmd.first, cmd.second);
        }
    }

    tree_.commit_model_changes();

    // save
    if (has_cmd(cmds, "save")) {
        save();
    }

    std::string show_index;
    if (has_cmd(cmds, "show", &show_index)) {
        actions["show"](show_index); // exit
    }
}

template <typename T>
void CommandLineAPI<T>::set_sink_file(const std::string& sink, ParameterSerdesType sink_type)
{
    sink_ = sink;
    sink_type_ = sink_type;

    try {
        load();
    } catch (const BadFile& e) {
        std::cerr << "load " << sink_ << " failure!" << e.what() << std::endl;
        save(); // 文件不存在, 将默认值序列化到文件中
        std::cout << "save " << sink_ << " with current value!" << std::endl;
    }

    // 使能参数变更通知
    tree_.enable_inform(true);

    // 监控文件变更
    monite_ = std::unique_ptr<FileMonite>(new FileMonite(sink_, std::bind(&CommandLineAPI::reload, this)));
    monite_->start();
}

template <typename T>
bool CommandLineAPI<T>::is_monitoring()
{
    return nullptr != monite_;
}

template <typename T>
ParameterSerdesType CommandLineAPI<T>::sink_type()
{
    return sink_type_;
}

template <typename T>
void CommandLineAPI<T>::load()
{
    if (sink_.empty()) {
        throw SinkfileNoSpecified(" for load");
    }

    std::fstream fin(sink_, std::ios::in);
    if (!fin) {
        throw BadFile("load " + sink_);
    }

    std::stringstream ss;
    ss << fin.rdbuf();
    std::string content = ss.str();

    {
        std::lock_guard<std::mutex> guard(sink_content_lock_);
        if (content == sink_content_) {
            std::cout << "with same content, no need to parse!" << std::endl;
            return;
        }
    }

    deserialize(content);

    {
        std::lock_guard<std::mutex> guard(sink_content_lock_);
        sink_content_.swap(content);
    }
}

template <typename T>
void CommandLineAPI<T>::save()
{
    if (sink_.empty()) {
        throw SinkfileNoSpecified(" for save");
    }

    std::fstream fout(sink_, std::ios::out | std::ios::trunc);
    if (!fout) {
        throw BadFile("save " + sink_);
    }

    auto content = serialize();

    {
        std::lock_guard<std::mutex> guard(sink_content_lock_);
        sink_content_.swap(content);
        fout << sink_content_;
    }
}

template <typename T>
void CommandLineAPI<T>::reload()
{
    try {
        std::cout << "reload " << sink_ << std::endl;
        load();
    } catch (ViolationException& e) {
        save();
        std::cerr << "sink file " << sink_ << " reload invalid parameter, write back to sink!" << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "sink file " << sink_ << " changed, load failure!" << e.what() << std::endl;
    }
}

template <typename T>
bool CommandLineAPI<T>::has_cmd(std::vector<std::pair<std::string, std::string>> cmds, const std::string& target, std::string* param)
{
    for (const auto& cmd : cmds) {
        if (cmd.first == target) {
            if (param) {
                *param = cmd.second;
            }
            return true;
        }
    }
    return false;
}

template <typename T, typename... APIS>
class ExtendedParameterTree : public ParameterTree<T>, public APIS... {
public:
    /// @brief 每次调用都创建一个新的实例
    /// @param root 实例名称
    /// @return 返回新创建的实例
    static std::shared_ptr<ExtendedParameterTree> create_instance(const std::string& root = "root")
    {
        return std::shared_ptr<ExtendedParameterTree>(new ExtendedParameterTree(root));
    }

    /// @brief 创建或返回已建实例(以实例名称作为索引)
    /// @param root 实例名称
    /// @return 返回新建或已建实例
    static std::shared_ptr<ExtendedParameterTree> get_or_create_instance(const std::string& root)
    {
        static std::mutex lock;
        static std::unordered_map<std::string, std::shared_ptr<ExtendedParameterTree>> instances;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto iter = instances.find(root);
            if (iter != instances.end()) {
                return iter->second;
            }
            return instances[root] = create_instance(root);
        }
    }

private:
    ExtendedParameterTree(const std::string& root)
        : ParameterTree<T>(root)
        , APIS(static_cast<ParameterTree<T>&>(*this))...
    {
    }

private:
    std::string serialize()
    {
        std::string out;

#if ENABLE_JSON
        if (this->sink_type() == ParameterSerdesType::PST_JSON) {
            out = this->to_json();
        }
#endif

#if ENABLE_YAML
        if (this->sink_type() == ParameterSerdesType::PST_YAML) {
            out = this->to_yaml();
        }
#endif

#if ENABLE_TOML
        if (this->sink_type() == ParameterSerdesType::PST_TOML) {
            out = this->to_toml();
        }
#endif

#if ENABLE_PROTOBUF
        if (this->sink_type() == ParameterSerdesType::PST_PBFMT) {
            out = this->to_pbtxt();
        }
#endif

        return out;
    }

    void deserialize(const std::string& in)
    {
#if ENABLE_YAML
        if (this->sink_type() == ParameterSerdesType::PST_YAML) {
            this->from_yaml(in);
        }
#endif

#if ENABLE_JSON
        if (this->sink_type() == ParameterSerdesType::PST_JSON) {
            this->from_json(in);
        }
#endif

#if ENABLE_TOML
        if (this->sink_type() == ParameterSerdesType::PST_TOML) {
            this->from_toml(in);
        }
#endif

#if ENABLE_PROTOBUF
        if (this->sink_type() == ParameterSerdesType::PST_PBFMT) {
            this->from_pbtxt(in);
        }
#endif
    }
};

template <typename T>
using SerdesTree = ExtendedParameterTree<T, CommandLineAPI<T>
#if ENABLE_JSON
    ,
    JsonAPI<T>
#endif
#if ENABLE_YAML
    ,
    YamlAPI<T>
#endif
#if ENABLE_TOML
    ,
    TomlAPI<T>
#endif
#if ENABLE_PROTOBUF
    ,
    ProtobufAPI<T>
#endif
    >;

template <typename T>
std::shared_ptr<SerdesTree<T>> create(const std::string& root = "root")
{
    return SerdesTree<T>::create_instance(root);
}

template <typename T>
std::shared_ptr<SerdesTree<T>> instance(const std::string& root)
{
    return SerdesTree<T>::get_or_create_instance(root);
}

template <typename T>
using entity = std::shared_ptr<SerdesTree<T>>;

}