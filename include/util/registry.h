#pragma once

/**
 * Simple registry implementation that uses static variables to
 * register object creators during program initialization time.
 */

// NB: This Registry works poorly when you have other namespaces.
// Make all macro invocations from inside the at namespace.
#include <functional>
#include <mutex>
#include <vector>

#include "logging/util/string_util.h"
#include "util/flat_hash.h"
#include "util/macros.h"

namespace serialization
{
template <class KeyType, typename Function>
class Registry
{
public:
    Registry() = default;

    void Register(const KeyType& key, Function f)
    {
        std::lock_guard<std::mutex> lock(register_mutex_);
        registry_[key] = f;
    }

    inline bool Has(const KeyType& key) { return (registry_.count(key) != 0); }

    template <class Arg1, class Arg2, class... Args>
    auto run(const KeyType& key, Arg1& arg1, Arg2* arg2, Args... args)
    {
        return registry_[key](arg1, arg2, args...);
    }

    template <class Arg1, class Arg2, class... Args>
    auto run(const KeyType& key, Arg1& arg1, Arg2& arg2, Args... args)
    {
        return registry_[key](arg1, arg2, args...);
    }

    /**
     * Returns the keys currently registered as a std::vector.
     */
    std::vector<KeyType> Keys() const
    {
        std::vector<KeyType> keys;
        for (const auto& it : registry_)
        {
            keys.push_back(it.first);
        }
        return keys;
    }

    Registry(const Registry&)                    = delete;
    Registry& operator=(const Registry& /*rhs*/) = delete;

private:
    quarisma::quarisma_map<KeyType, Function> registry_{};
    std::mutex                                register_mutex_;
};

template <class KeyType, typename Function>
class Registerer
{
public:
    explicit Registerer(const KeyType& key, Registry<KeyType, Function>* registry, Function method)
    {
        registry->Register(key, method);
    }
};
/**
 * @brief A template class that allows one to register classes by keys.
 *
 * The keys are usually a std::string specifying the name, but can be anything
 * that can be used in a std::map.
 *
 * You should most likely not use the Registry class explicitly, but use the
 * helper macros below to declare specific registries as well as registering
 * objects.
 */
namespace creator
{
template <class KeyType, class ReturnType, class... Args>
class Registry
{
public:
    using Function = std::function<ReturnType(Args...)>;

    Registry() = default;

    void Register(const KeyType& key, Function f)
    {
        std::lock_guard<std::mutex> lock(register_mutex_);
        registry_[key] = f;
    }

    inline bool Has(const KeyType& key) { return (registry_.count(key) != 0); }

    ReturnType run(const KeyType& key, Args... args)
    {
        if (registry_.count(key) == 0)
        {
            // Returns nullptr if the key is not registered.
            return nullptr;
        }
        return registry_[key](args...);
    }

    /**
     * Returns the keys currently registered as a std::vector.
     */
    std::vector<KeyType> Keys() const
    {
        std::vector<KeyType> keys;
        for (const auto& it : registry_)
        {
            keys.push_back(it.first);
        }
        return keys;
    }

    Registry(const Registry&)                    = delete;
    Registry& operator=(const Registry& /*rhs*/) = delete;

private:
    quarisma::quarisma_map<KeyType, Function> registry_{};
    std::mutex                                register_mutex_;
};

template <class KeyType, class ReturnType, class... Args>
class Registerer
{
public:
    explicit Registerer(
        const KeyType&                                            key,
        Registry<KeyType, ReturnType, Args...>*                   registry,
        typename Registry<KeyType, ReturnType, Args...>::Function method)
    {
        registry->Register(key, method);
    }

    template <class DerivedType>
    static ReturnType DefaultCreator(Args... args)
    {
        return ReturnType(new DerivedType(args...));
    }
};
}  // namespace creator

#define SERIALIZATION_DECLARE_FUNCTION_REGISTRY(RegistryName, Function) \
    serialization::Registry<std::string, Function>* RegistryName();     \
    using Registerer##RegistryName = serialization::Registerer<std::string, Function>;

#define SERIALIZATION_DEFINE_FUNCTION_REGISTRY(RegistryName, Function)                \
    serialization::Registry<std::string, Function>* RegistryName()                    \
    {                                                                                 \
        static auto* registry = new serialization::Registry<std::string, Function>(); \
        return registry;                                                              \
    }

#define SERIALIZATION_REGISTER_FUNCTION(RegistryName, type, Function)                   \
    static Registerer##RegistryName SERIALIZATION_ANONYMOUS_VARIABLE(g_##RegistryName)( \
        logging::demangle(typeid(type).name()), RegistryName(), Function);

#define SERIALIZATION_DECLARE_TYPED_REGISTRY(RegistryName, KeyType, ObjectType, PtrType, ...)      \
    serialization::creator::Registry<KeyType, PtrType<ObjectType>, ##__VA_ARGS__>* RegistryName(); \
    using Registerer##RegistryName =                                                               \
        serialization::creator::Registerer<KeyType, PtrType<ObjectType>, ##__VA_ARGS__>;

#define SERIALIZATION_DEFINE_TYPED_REGISTRY(RegistryName, KeyType, ObjectType, PtrType, ...)      \
    serialization::creator::Registry<KeyType, PtrType<ObjectType>, ##__VA_ARGS__>* RegistryName() \
    {                                                                                             \
        static auto* registry =                                                                   \
            new serialization::creator::Registry<KeyType, PtrType<ObjectType>, ##__VA_ARGS__>();  \
        return registry;                                                                          \
    }

// The __VA_ARGS__ below allows one to specify a templated
// creator with comma in its templated arguments.
#define SERIALIZATION_REGISTER_TYPED_CREATOR(RegistryName, key, ...)                    \
    static Registerer##RegistryName SERIALIZATION_ANONYMOUS_VARIABLE(g_##RegistryName)( \
        key, RegistryName(), ##__VA_ARGS__);

#define SERIALIZATION_REGISTER_TYPED_CLASS(RegistryName, key, ...)                      \
    static Registerer##RegistryName SERIALIZATION_ANONYMOUS_VARIABLE(g_##RegistryName)( \
        key, RegistryName(), Registerer##RegistryName::DefaultCreator<__VA_ARGS__>);

// SERIALIZATION_DECLARE_REGISTRY and SERIALIZATION_DEFINE_REGISTRY are hard-wired to use
// std::string as the key type, because that is the most commonly used cases.
#define SERIALIZATION_DECLARE_REGISTRY(RegistryName, ObjectType, ...) \
    SERIALIZATION_DECLARE_TYPED_REGISTRY(                             \
        RegistryName, std::string, ObjectType, std::unique_ptr, ##__VA_ARGS__)

#define SERIALIZATION_DEFINE_REGISTRY(RegistryName, ObjectType, ...) \
    SERIALIZATION_DEFINE_TYPED_REGISTRY(                             \
        RegistryName, std::string, ObjectType, std::unique_ptr, ##__VA_ARGS__)

// SERIALIZATION_REGISTER_CREATOR and SERIALIZATION_REGISTER_CLASS are hard-wired to use std::string
// as the key
// type, because that is the most commonly used cases.
#define SERIALIZATION_REGISTER_CREATOR(RegistryName, key, ...) \
    SERIALIZATION_REGISTER_TYPED_CREATOR(RegistryName, #key, __VA_ARGS__)

#define SERIALIZATION_REGISTER_CLASS(RegistryName, key, ...) \
    SERIALIZATION_REGISTER_TYPED_CLASS(RegistryName, #key, __VA_ARGS__)

// SERIALIZATION_DECLARE_SHARED_REGISTRY and SERIALIZATION_DEFINE_SHARED_REGISTRY use std::shared_ptr
// instead of std::unique_ptr for shared ownership semantics
#define SERIALIZATION_DECLARE_SHARED_REGISTRY(RegistryName, ObjectType, ...) \
    SERIALIZATION_DECLARE_TYPED_REGISTRY(                                    \
        RegistryName, std::string, ObjectType, std::shared_ptr, ##__VA_ARGS__)

#define SERIALIZATION_DEFINE_SHARED_REGISTRY(RegistryName, ObjectType, ...) \
    SERIALIZATION_DEFINE_TYPED_REGISTRY(                                    \
        RegistryName, std::string, ObjectType, std::shared_ptr, ##__VA_ARGS__)

}  // namespace serialization
