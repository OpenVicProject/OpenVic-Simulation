from pathlib import Path


def generate_memory_config_header(target, source, env):
    header_file_path = Path(str(target[0]))

    header = []

    header.append("// THIS FILE IS GENERATED. EDITS WILL BE LOST.")
    header.append("")

    header += """
// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#ifndef FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
#error "do not include this file directly, use config.hpp"
#endif

#include <cstddef>

//=== options ===//
// clang-format off
""".split("\n")

    for key, val in env.config_data.items():
        if isinstance(val, bool) and val:
            val = 1
        elif isinstance(val, bool) and not val:
            val = 0
        header.append(f"#define {key} {val}")

    header.append("// clang-format on")
    header.append("")

    with header_file_path.open("w+", encoding="utf-8") as header_file:
        header_file.write("\n".join(header))


def generate_memory_container_size_header(target, source, env):
    header_file_path = Path(str(target[0]))

    header = []

    header.append("// THIS FILE IS GENERATED. EDITS WILL BE LOST.")
    header.append("")

    header += """
namespace detail
{
    template <std::size_t Alignment>
    struct alignment_type
    {
        using type = void;
        static_assert(Alignment == Alignment);
    };

    template <>
    struct alignment_type<1>
    {
        using type = char;
        static_assert(alignof(type) == 1);
    };

    template <>
    struct alignment_type<2>
    {
        using type = short;
        static_assert(alignof(type) == 2);
    };

    template <>
    struct alignment_type<4>
    {
        using type = int;
        static_assert(alignof(type) == 4);
    };

    template <>
    struct alignment_type<8>
    {
        using type = std::conditional_t<alignof(long) == 8, long, long double>;
        static_assert(alignof(type) == 8);
    };

    template <>
    struct alignment_type<16>
    {
        using type =
            std::conditional_t<alignof(long double) == 16, long double, alignment_type<0>::type>;
    };

    template <std::size_t Alignment>
    using alignment_type_t = alignment_type<Alignment>::type;

    template <typename InitialType, typename T, bool SubtractTSize = true>
    static consteval std::size_t calculate_node_size_by_type()
    {
        static_assert(!std::is_same<InitialType, T>::value && (sizeof(InitialType) != sizeof(T)));
        static_assert(sizeof(T) > sizeof(InitialType));

        return sizeof(T) - (SubtractTSize ? sizeof(InitialType) : 0);
    }

    template <std::size_t Alignment, typename T, bool SubtractTSize = true>
    static consteval std::size_t calculate_node_size()
    {
        return calculate_node_size_by_type<alignment_type_t<Alignment>, T, SubtractTSize>();
    }

    template <std::size_t Alignment>
    using allocator_type = std::allocator<alignment_type_t<Alignment>>;
}
""".split("\n")

    containers = {
        "forward_list": [
            "calculate_node_size<Alignment, std::__1::__forward_list_node<alignment_type_t<Alignment>, void*>>()",
            "calculate_node_size<Alignment, std::_Fwd_list_node<alignment_type_t<Alignment>>>()",
            "calculate_node_size<Alignment, std::_Flist_node<alignment_type_t<Alignment>, void*>>()",
        ],
        "list": [
            "calculate_node_size<Alignment, std::__1::__list_node<alignment_type_t<Alignment>, void*>>()",
            "calculate_node_size<Alignment, std::_List_node<alignment_type_t<Alignment>>>()",
            "calculate_node_size<Alignment, std::_List_node<alignment_type_t<Alignment>, void*>>()",
        ],
        "set": [
            "calculate_node_size<Alignment, std::__1::__tree_node<alignment_type_t<Alignment>, void*>>()",
            "calculate_node_size<Alignment, std::_Rb_tree_node<alignment_type_t<Alignment>>>()",
            "calculate_node_size<Alignment, std::_Tree_node<alignment_type_t<Alignment>, void*>>()",
        ],
        "multiset": [],
        "unordered_set": [
            "calculate_node_size<Alignment, std::__1::__hash_node<alignment_type_t<Alignment>, void*>>()",
            "calculate_node_size<Alignment, typename std::__detail::_Hash_node<alignment_type_t<Alignment>, true>>()",
            "calculate_node_size<Alignment, std::_List_node<alignment_type_t<Alignment>, void*>>()",
        ],
        "unordered_multiset": [],
        "map": [
            """
calculate_node_size_by_type<
    std::__1::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::__1::__tree_node<
        std::__1::__value_type<alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
        void*>>()
""",
            """
calculate_node_size_by_type<
    std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::_Rb_tree_node<
        std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>>>()
""",
            """
calculate_node_size_by_type<
    std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::_Tree_node<
        std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
        void*>>()
""",
        ],
        "multimap": [],
        "unordered_map": [
            """
calculate_node_size_by_type<
    std::__1::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::__1::__hash_node<std::__1::__hash_value_type<alignment_type_t<Alignment>,
                                                    alignment_type_t<Alignment>>,
                        void*>>()
""",
            """
calculate_node_size_by_type<
    std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::__detail::_Hash_node<
        std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
        true>>()
""",
            """
calculate_node_size_by_type<
    std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
    std::_List_node<
        std::pair<const alignment_type_t<Alignment>, alignment_type_t<Alignment>>,
        void*>>()
""",
        ],
        "unordered_multimap": [],
        "shared_ptr_stateless": [
            """
calculate_node_size<Alignment,
    std::__1::__shared_ptr_emplace<
        alignment_type_t<Alignment>,
        std::allocator<alignment_type_t<Alignment>>>,
    false>()
""",
            """
calculate_node_size<Alignment,
    std::_Sp_counted_ptr_inplace<alignment_type_t<Alignment>,
                                std::allocator<alignment_type_t<Alignment>>,
                                __gnu_cxx::_S_atomic>,
    false>()
""",
            """
calculate_node_size<
    Alignment,
    std::_Ref_count_obj_alloc3<std::remove_cv_t<alignment_type_t<Alignment>>,
                                allocator_type<Alignment>>,
    false>()
""",
        ],
        "shared_ptr_stateful": [
            """
calculate_node_size<Alignment,
    std::__1::__shared_ptr_emplace<
        alignment_type_t<Alignment>,
        std::pmr::polymorphic_allocator<alignment_type_t<Alignment>>>,
    false>()
""",
            """
calculate_node_size<Alignment,
    std::_Sp_counted_ptr_inplace<
        alignment_type_t<Alignment>,
        std::pmr::polymorphic_allocator<alignment_type_t<Alignment>>,
        __gnu_cxx::_S_atomic>,
    false>()
""",
            """
calculate_node_size<Alignment,
    std::_Ref_count_obj_alloc3<
        std::remove_cv_t<alignment_type_t<Alignment>>,
        std::pmr::polymorphic_allocator<alignment_type_t<Alignment>>>,
    false>()
""",
        ],
    }

    container_names = list(containers.keys())
    for i, container_name in enumerate(container_names):
        data = containers[container_name]
        if len(data) == 0:
            data = containers[container_names[i - 1]]

        header += f'''namespace detail
{{
#ifdef _LIBCPP_VERSION
    template <std::size_t Alignment>
    struct {container_name}_node_size
    : std::integral_constant<
          std::size_t,{data[0]}>
    {{
    }};
#elif defined(__GLIBCXX__)
    template <std::size_t Alignment>
    struct {container_name}_node_size
    : std::integral_constant<
          std::size_t,{data[1]}>
    {{
    }};
#elif defined(_MSC_VER)
    template <std::size_t Alignment>
    struct {container_name}_node_size
    : std::integral_constant<
          std::size_t,{data[2]}>
    {{
    }};
#else
    template <std::size_t Alignment>
    struct {container_name}_node_size
    : std::integral_constant<
          std::size_t,0>
    {{
        static_assert(Alignment == Alignment, "{container_name}_node_size not supported.");
    }};
#endif
}}

template <typename T>
struct {container_name}_node_size
: std::integral_constant<std::size_t,
    detail::round_up_to_multiple_of_alignment(detail::{container_name}_node_size<alignof(T)>::value + sizeof(T), alignof(void*))>
{{}};
'''.split("\n")

    with header_file_path.open("w+", encoding="utf-8") as header_file:
        header_file.write("\n".join(header))
