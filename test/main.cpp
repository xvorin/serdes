#include "common.h"

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    g_wp = xvorin::serdes::create<TestParam>("root", "test.yaml");
    std::cout << g_wp->to_pbdef("v2x") << std::endl;
    std::cout << g_wp->to_pbbin() << std::endl;
    // std::cout << g_wp->debug_string() << std::endl;

    auto g_wp2 = xvorin::serdes::create<TestParam>("root2");
    g_wp2->from_pbbin(g_wp->to_pbbin());
    std::cout << g_wp2->debug_string() << std::endl;
    // std::cout << g_wp->debug_string() << std::endl;

    // std::cout << g_wp->value().top_string << std::endl;

    // for (auto& x : g_wp->value().top_list_string) {
    //     std::cout << "top_list_string:" << x << std::endl;
    // }

    // for (auto& x : g_wp->value().top_uset_int) {
    //     std::cout << "top_uset_int:" << x << std::endl;
    // }

    // for (auto& x : g_wp->value().top_vector_string) {
    //     std::cout << "top_vector_string:" << x << std::endl;
    // }

    // for (auto& x : g_wp->value().top_set_string) {
    //     std::cout << "top_set_string:" << x << std::endl;
    // }

    // for (auto& x : g_wp->value().top_map_string) {
    //     std::cout << "top_map_string:" << x.first << "," << x.second << std::endl;
    // }

    // for (auto& x : g_wp->value().top_umap_string) {
    //     std::cout << "top_umap_string:" << x.first << "," << x.second << std::endl;
    // }

    // for (auto& x : g_wp->value().top_map_int64_string) {
    //     std::cout << "top_map_int64_string:" << x.first << "," << x.second << std::endl;
    // }

    return RUN_ALL_TESTS();
}