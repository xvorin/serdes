///////////////////////////////////////////////// for object member /////////////////////////////////////
#define CHECK_STRUCT_BASIC_VALUE(key)                            \
    ASSERT_EQ(key, g_wp->parameter<decltype(key)>(#key)->value); \
    ASSERT_EQ(#key, g_wp->parameter<decltype(key)>(#key)->index());

////////////////////////////////////////////// for list & vector & set //////////////////////////////////
#define CHECK_STL_SIZE(list)                                             \
    {                                                                    \
        ASSERT_EQ(list.size(), g_wp->parameter(#list)->children_size()); \
        ASSERT_EQ(#list, g_wp->parameter(#list)->index());               \
    }

#define CHECK_STL_SIZE2(list)                                              \
    {                                                                      \
        size_t i = 0;                                                      \
        for (auto& item : list) {                                          \
            const auto key = std::string(#list ".") + std::to_string(i++); \
            ASSERT_EQ(item.size(), g_wp->parameter(key)->children_size()); \
            ASSERT_EQ(key, g_wp->parameter(key)->index());                 \
        }                                                                  \
    }

#define CHECK_STL_SIZE3(list)                                                  \
    {                                                                          \
        size_t i = 0;                                                          \
        for (auto& iteml : list) {                                             \
            size_t j = 0;                                                      \
            for (auto& item : iteml) {                                         \
                const auto key = std::string(#list)                            \
                                     .append(".")                              \
                                     .append(std::to_string(i))                \
                                     .append(".")                              \
                                     .append(std::to_string(j));               \
                ASSERT_EQ(item.size(), g_wp->parameter(key)->children_size()); \
                ASSERT_EQ(key, g_wp->parameter(key)->index());                 \
                j++;                                                           \
            }                                                                  \
            i++;                                                               \
        }                                                                      \
    }

#define CHECK_STL_STRUCT_BASIC_VALUE(list, m)                  \
    {                                                          \
        size_t i = 0;                                          \
        for (auto& item : list) {                              \
            using T = decltype(item.m);                        \
            const auto key = std::string(#list)                \
                                 .append(".")                  \
                                 .append(std::to_string(i++))  \
                                 .append("." #m);              \
            ASSERT_EQ(item.m, g_wp->parameter<T>(key)->value); \
            ASSERT_EQ(key, g_wp->parameter(key)->index());     \
        }                                                      \
    }

#define CHECK_STL_STRUCT_BASIC_VALUE2(list, m)                     \
    {                                                              \
        size_t i = 0;                                              \
        for (auto& iteml : list) {                                 \
            size_t j = 0;                                          \
            for (auto& item : iteml) {                             \
                using T = decltype(item.m);                        \
                const auto key = std::string(#list)                \
                                     .append(".")                  \
                                     .append(std::to_string(i))    \
                                     .append(".")                  \
                                     .append(std::to_string(j))    \
                                     .append("." #m);              \
                ASSERT_EQ(item.m, g_wp->parameter<T>(key)->value); \
                ASSERT_EQ(key, g_wp->parameter(key)->index());     \
                j++;                                               \
            }                                                      \
            i++;                                                   \
        }                                                          \
    }

#define CHECK_STL_STRUCT_BASIC_VALUE3(list, m)                         \
    {                                                                  \
        size_t i = 0;                                                  \
        for (auto& itemll : list) {                                    \
            size_t j = 0;                                              \
            for (auto& iteml : itemll) {                               \
                size_t z = 0;                                          \
                for (auto& item : iteml) {                             \
                    using T = decltype(item.m);                        \
                    auto key = std::string(#list)                      \
                                   .append(".")                        \
                                   .append(std::to_string(i))          \
                                   .append(".")                        \
                                   .append(std::to_string(j))          \
                                   .append(".")                        \
                                   .append(std::to_string(z))          \
                                   .append("." #m);                    \
                    ASSERT_EQ(item.m, g_wp->parameter<T>(key)->value); \
                    ASSERT_EQ(key, g_wp->parameter(key)->index());     \
                    z++;                                               \
                }                                                      \
                j++;                                                   \
            }                                                          \
            i++;                                                       \
        }                                                              \
    }

#define CHECK_STL_BASIC_VALUE(list)                                                                  \
    {                                                                                                \
        size_t i = 0;                                                                                \
        for (auto& item : list) {                                                                    \
            using T = typename std::remove_const<std::remove_reference<decltype(item)>::type>::type; \
            const auto key = std::string(#list)                                                      \
                                 .append(".")                                                        \
                                 .append(std::to_string(i++));                                       \
            ASSERT_EQ(item, g_wp->parameter<T>(key)->value);                                         \
            ASSERT_EQ(key, g_wp->parameter(key)->index());                                           \
        }                                                                                            \
    }

#define CHECK_STL_BASIC_VALUE2(list)                                                                     \
    {                                                                                                    \
        size_t i = 0;                                                                                    \
        for (auto& iteml : list) {                                                                       \
            size_t j = 0;                                                                                \
            for (auto& item : iteml) {                                                                   \
                using T = typename std::remove_const<std::remove_reference<decltype(item)>::type>::type; \
                const auto key = std::string(#list)                                                      \
                                     .append(".")                                                        \
                                     .append(std::to_string(i))                                          \
                                     .append(".")                                                        \
                                     .append(std::to_string(j));                                         \
                ASSERT_EQ(item, g_wp->parameter<T>(key)->value);                                         \
                ASSERT_EQ(key, g_wp->parameter(key)->index());                                           \
                j++;                                                                                     \
            }                                                                                            \
            i++;                                                                                         \
        }                                                                                                \
    }

#define CHECK_STL_BASIC_VALUE3(list)                                                                         \
    {                                                                                                        \
        size_t i = 0;                                                                                        \
        for (auto& itemll : list) {                                                                          \
            size_t j = 0;                                                                                    \
            for (auto& iteml : itemll) {                                                                     \
                size_t z = 0;                                                                                \
                for (auto& item : iteml) {                                                                   \
                    using T = typename std::remove_const<std::remove_reference<decltype(item)>::type>::type; \
                    const auto key = std::string(#list)                                                      \
                                         .append(".")                                                        \
                                         .append(std::to_string(i))                                          \
                                         .append(".")                                                        \
                                         .append(std::to_string(j))                                          \
                                         .append(".")                                                        \
                                         .append(std::to_string(z));                                         \
                    ASSERT_EQ(item, g_wp->parameter<T>(key)->value);                                         \
                    ASSERT_EQ(key, g_wp->parameter(key)->index());                                           \
                    z++;                                                                                     \
                }                                                                                            \
                j++;                                                                                         \
            }                                                                                                \
            i++;                                                                                             \
        }                                                                                                    \
    }

///////////////////////////////////////////////////// for map /////////////////////////////////////////
#define CHECK_MAP_SIZE(m)                                          \
    {                                                              \
        ASSERT_EQ(m.size(), g_wp->parameter(#m)->children_size()); \
        ASSERT_EQ(#m, g_wp->parameter(#m)->index());               \
    }

#define CHECK_MAP_SIZE2(m)                                                        \
    {                                                                             \
        for (auto& item : m) {                                                    \
            auto key = std::string(#m)                                            \
                           .append(".")                                           \
                           .append(item.first);                                   \
            ASSERT_EQ(item.second.size(), g_wp->parameter(key)->children_size()); \
            ASSERT_EQ(key, g_wp->parameter(key)->index());                        \
        }                                                                         \
    }

#define CHECK_MAP_SIZE3(m)                                                            \
    {                                                                                 \
        for (auto& iteml : m) {                                                       \
            for (auto& item : iteml.second) {                                         \
                auto key = std::string(#m)                                            \
                               .append(".")                                           \
                               .append(iteml.first)                                   \
                               .append(".")                                           \
                               .append(item.first);                                   \
                ASSERT_EQ(item.second.size(), g_wp->parameter(key)->children_size()); \
                ASSERT_EQ(key, g_wp->parameter(key)->index());                        \
            }                                                                         \
        }                                                                             \
    }

#define CHECK_MAP_BASIC_VALUE(m)                                    \
    {                                                               \
        using T = typename decltype(m)::mapped_type;                \
        for (auto& item : m) {                                      \
            auto key = std::string(#m)                              \
                           .append(".")                             \
                           .append(item.first);                     \
            ASSERT_EQ(item.second, g_wp->parameter<T>(key)->value); \
            ASSERT_EQ(key, g_wp->parameter(key)->index());          \
        }                                                           \
    }

#define CHECK_MAP_BASIC_VALUE2(m)                                       \
    {                                                                   \
        using T = typename decltype(m)::mapped_type::mapped_type;       \
        for (auto& iteml : m) {                                         \
            for (auto& item : iteml.second) {                           \
                auto key = std::string(#m)                              \
                               .append(".")                             \
                               .append(iteml.first)                     \
                               .append(".")                             \
                               .append(item.first);                     \
                ASSERT_EQ(item.second, g_wp->parameter<T>(key)->value); \
                ASSERT_EQ(key, g_wp->parameter(key)->index());          \
            }                                                           \
        }                                                               \
    }

#define CHECK_MAP_BASIC_VALUE3(m)                                              \
    {                                                                          \
        using T = typename decltype(m)::mapped_type::mapped_type::mapped_type; \
        for (auto& itemll : m) {                                               \
            for (auto& iteml : itemll.second) {                                \
                for (auto& item : iteml.second) {                              \
                    auto key = std::string(#m)                                 \
                                   .append(".")                                \
                                   .append(itemll.first)                       \
                                   .append(".")                                \
                                   .append(iteml.first)                        \
                                   .append(".")                                \
                                   .append(item.first);                        \
                    ASSERT_EQ(item.second, g_wp->parameter<T>(key)->value);    \
                    ASSERT_EQ(key, g_wp->parameter(key)->index());             \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }
