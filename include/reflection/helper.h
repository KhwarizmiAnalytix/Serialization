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

#include <new>      // for operator new
#include <utility>  // for forward

#include "util/pointer.h"  // for shared_ptr, unique_ptr

namespace serialization
{
namespace access
{
struct serializer
{
    /**
     * Allows placement construction of types.
     */
    template <typename Item, typename... Arguments>
    static auto placement_new(void* pAddress, Arguments&&... arguments) noexcept(
        noexcept(Item(std::forward<Arguments>(arguments)...)))
    {
        return ::new (pAddress) Item(std::forward<Arguments>(arguments)...);
    }

    /**
     * Allows destruction of types.
     */
    template <typename Item>
    static void destruct(Item& item) noexcept
    {
        item.~Item();
    }

    // use in case the object default constructor is not public
    template <typename T>
    inline static auto make_ptr()
    {
        return std::unique_ptr<T>(new T());
    }

    // use in case the object default constructor is public
    template <typename T>
    inline static auto make_shard_ptr()
    {
        return std::shared_ptr<T>(new T());
    }

    template <typename T>
    inline static void initialize(T& obj)
    {
        obj.initialize();
    }

    template <typename T>
    constexpr static auto tuple() -> decltype(T::properties())
    {
        return T::properties();
    }
};
}  // namespace access
}  // namespace serialization
