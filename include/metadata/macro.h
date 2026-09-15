/* Copyright 2018 The Serialization Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#pragma once

#include <string>

#include "codecs/native.h"
#include "metadata/provider.h"

namespace serialization
{
/******************************************/
/* arg list expand macro, now support 120 args */
#define MAKE_T_ARG_LIST_1(op, T, arg, ...) op(T, arg)
#define MAKE_T_ARG_LIST_2(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_1(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_3(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_2(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_4(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_3(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_5(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_4(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_6(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_5(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_7(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_6(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_8(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_7(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_9(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_8(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_10(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_9(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_11(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_10(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_12(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_11(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_13(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_12(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_14(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_13(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_15(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_14(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_16(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_15(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_17(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_16(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_18(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_17(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_19(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_18(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_20(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_19(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_21(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_20(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_22(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_21(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_23(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_22(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_24(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_23(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_25(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_24(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_26(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_25(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_27(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_26(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_28(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_27(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_29(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_28(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_30(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_29(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_31(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_30(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_32(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_31(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_33(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_32(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_34(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_33(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_35(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_34(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_36(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_35(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_37(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_36(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_38(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_37(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_39(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_38(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_40(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_39(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_41(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_40(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_42(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_41(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_43(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_42(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_44(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_43(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_45(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_44(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_46(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_45(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_47(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_46(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_48(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_47(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_49(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_48(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_50(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_49(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_51(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_50(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_52(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_51(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_53(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_52(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_54(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_53(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_55(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_54(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_56(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_55(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_57(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_56(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_58(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_57(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_59(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_58(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_60(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_59(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_61(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_60(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_62(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_61(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_63(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_62(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_64(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_63(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_65(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_64(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_66(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_65(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_67(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_66(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_68(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_67(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_69(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_68(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_70(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_69(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_71(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_70(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_72(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_71(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_73(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_72(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_74(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_73(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_75(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_74(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_76(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_75(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_77(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_76(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_78(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_77(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_79(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_78(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_80(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_79(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_81(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_80(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_82(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_81(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_83(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_82(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_84(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_83(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_85(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_84(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_86(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_85(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_87(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_86(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_88(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_87(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_89(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_88(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_90(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_89(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_91(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_90(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_92(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_91(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_93(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_92(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_94(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_93(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_95(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_94(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_96(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_95(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_97(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_96(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_98(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_97(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_99(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_98(op, T, __VA_ARGS__))
#define MAKE_T_ARG_LIST_100(op, T, arg, ...) \
    op(T, arg), MACRO_EXPAND(MAKE_T_ARG_LIST_99(op, T, __VA_ARGS__))

#define RSEQ_N()                                                                                 \
    100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, \
        77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56,  \
        55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,  \
        33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,  \
        11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define ARG_N( \
    _1,        \
    _2,        \
    _3,        \
    _4,        \
    _5,        \
    _6,        \
    _7,        \
    _8,        \
    _9,        \
    _10,       \
    _11,       \
    _12,       \
    _13,       \
    _14,       \
    _15,       \
    _16,       \
    _17,       \
    _18,       \
    _19,       \
    _20,       \
    _21,       \
    _22,       \
    _23,       \
    _24,       \
    _25,       \
    _26,       \
    _27,       \
    _28,       \
    _29,       \
    _30,       \
    _31,       \
    _32,       \
    _33,       \
    _34,       \
    _35,       \
    _36,       \
    _37,       \
    _38,       \
    _39,       \
    _40,       \
    _41,       \
    _42,       \
    _43,       \
    _44,       \
    _45,       \
    _46,       \
    _47,       \
    _48,       \
    _49,       \
    _50,       \
    _51,       \
    _52,       \
    _53,       \
    _54,       \
    _55,       \
    _56,       \
    _57,       \
    _58,       \
    _59,       \
    _60,       \
    _61,       \
    _62,       \
    _63,       \
    _64,       \
    _65,       \
    _66,       \
    _67,       \
    _68,       \
    _69,       \
    _70,       \
    _71,       \
    _72,       \
    _73,       \
    _74,       \
    _75,       \
    _76,       \
    _77,       \
    _78,       \
    _79,       \
    _80,       \
    _81,       \
    _82,       \
    _83,       \
    _84,       \
    _85,       \
    _86,       \
    _87,       \
    _88,       \
    _89,       \
    _90,       \
    _91,       \
    _92,       \
    _93,       \
    _94,       \
    _95,       \
    _96,       \
    _97,       \
    _98,       \
    _99,       \
    _100,      \
    N,         \
    ...)       \
    N

#define MACRO_EXPAND(...) __VA_ARGS__
#define APPLY_VARIADIC_MACRO(macro, ...) MACRO_EXPAND(macro(__VA_ARGS__))

#define REFLECTION(T, t) serialization::reflection(&T::t, #t)

// note use MACRO_CONCAT like A##_##B direct may cause marco expand error
#define MACRO_CONCAT(A, B) MACRO_CONCAT1(A, B)
#define MACRO_CONCAT1(A, B) A##_##B

#define MAKE_T_ARG_LIST(N, op, T, arg, ...) \
    MACRO_CONCAT(MAKE_T_ARG_LIST, N)(op, T, arg, __VA_ARGS__)

#define GET_ARG_COUNT_INNER(...) MACRO_EXPAND(ARG_N(__VA_ARGS__))
#define GET_ARG_COUNT(...) GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

#define REFLECTION_META_DATA_IMPL(T, ...)    \
    constexpr static auto properties()       \
    {                                        \
        return std::make_tuple(__VA_ARGS__); \
    }

#define REFLECTION_META_DATA_IMPL_EMPTY(T)                                \
    constexpr static auto properties()                                    \
    {                                                                     \
        return std::make_tuple(serialization::reflection_no_member<T>()); \
    }

#define MAKE_META_DATA(T, N, ...) \
    REFLECTION_META_DATA_IMPL(T, MAKE_T_ARG_LIST(N, REFLECTION, T, __VA_ARGS__))

#define SERIALIZATION_MACRO(T, ...)                             \
    MAKE_META_DATA(T, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__); \
    friend struct serialization::access::serializer;            \
    friend struct serialization::generated::metadata<T>;

#define SERIALIZATION_MACRO_EMPTY(T)                 \
    REFLECTION_META_DATA_IMPL_EMPTY(T);              \
    friend struct serialization::access::serializer; \
    friend struct serialization::generated::metadata<T>;

#define SERIALIZATION_MACRO_TEMPLATE(T, ...)                    \
    MAKE_META_DATA(T, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__); \
    friend struct serialization::access::serializer;            \
    friend struct serialization::generated::metadata<T>;

// Helper macro to combine parent and derived properties
#define REFLECTION_META_DATA_DERIVED_IMPL(DerivedClass, ParentClass, ...) \
    constexpr static auto properties()                                    \
    {                                                                     \
        return std::tuple_cat(                                            \
            serialization::access::serializer::tuple<ParentClass>(),      \
            std::make_tuple(__VA_ARGS__));                                \
    }

// Macro for derived classes that automatically includes parent properties
#define SERIALIZATION_MACRO_DERIVED(DerivedClass, ParentClass, ...)                          \
    REFLECTION_META_DATA_DERIVED_IMPL(                                                       \
        DerivedClass,                                                                        \
        ParentClass,                                                                         \
        MAKE_T_ARG_LIST(GET_ARG_COUNT(__VA_ARGS__), REFLECTION, DerivedClass, __VA_ARGS__)); \
    friend struct serialization::access::serializer;                                         \
    friend struct serialization::generated::metadata<DerivedClass>;
}  // namespace serialization
