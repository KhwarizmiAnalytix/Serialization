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

#include <string_view>
#include <type_traits>
#include <utility>

namespace serialization
{

// Helper to iterate over integer sequences
template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f)
{
    (f(std::integral_constant<T, S>{}), ...);
}

// Reflection for class members
template <typename Class, typename T>
class reflection_impl
{
public:
    using class_type   = Class;
    using member_type  = T;
    using pointer_type = T Class::*;

    constexpr reflection_impl(
        pointer_type member, std::string_view name, std::string_view description = "") noexcept
        : member_(member), name_(name), description_(description)
    {
    }

    constexpr pointer_type     member() const noexcept { return member_; }
    constexpr std::string_view name() const noexcept { return name_; }
    constexpr std::string_view description() const noexcept { return description_; }

private:
    pointer_type     member_;
    std::string_view name_;
    std::string_view description_;
};

// Factory function
template <typename Class, typename T>
constexpr auto reflection(
    T Class::* member, std::string_view name, std::string_view description = "") noexcept
{
    return reflection_impl<Class, T>{member, name, description};
}

// For empty classes
template <typename Class>
class reflection_empty
{
public:
    using class_type = Class;

    constexpr reflection_empty(std::string_view name = "") noexcept : name_(name) {}

    constexpr std::string_view name() const noexcept { return name_; }

private:
    std::string_view name_;
};

template <typename Class>
constexpr auto reflection_no_member(std::string_view name = "") noexcept
{
    return reflection_empty<Class>{name};
}
}  // namespace serialization