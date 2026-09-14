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


#include <filesystem>
#include <string>
#include <vector>

#include "common/archiver_wrapper.h"
#include "serialization_impl.h"
#include "util/export.h"
#include "util/multi_process_stream.h"
#include "util/pointer.h"
#include "util/registry.h"

namespace serialization
{
#define COMMA ,
#define SERIALIZATION_REGISTER_DERIVED_SERIALIZATION(type)                                        \
    static serialization::RegistererJsonSerializationRegistry SERIALIZATION_ANONYMOUS_VARIABLE(   \
        g_JsonSerializationRegistry)(                                                             \
        logging::demangle(typeid(type).name()),                                                   \
        serialization::JsonSerializationRegistry(),                                               \
        &serialization::register_serializer_impl<serialization::json COMMA type>);                \
    static serialization::RegistererXmlSerializationRegistry SERIALIZATION_ANONYMOUS_VARIABLE(    \
        g_XmlSerializationRegistry)(                                                              \
        logging::demangle(typeid(type).name()),                                                   \
        serialization::XmlSerializationRegistry(),                                                \
        &serialization::register_serializer_impl<pugi::xml_node COMMA type>);                     \
    static serialization::RegistererBinarySerializationRegistry SERIALIZATION_ANONYMOUS_VARIABLE( \
        g_BinarySerializationRegistry)(                                                           \
        logging::demangle(typeid(type).name()),                                                   \
        serialization::BinarySerializationRegistry(),                                             \
        &serialization::register_serializer_impl<serialization::multi_process_stream COMMA type>);

namespace serialization_impl
{
class SERIALIZATION_VISIBILITY access
{
public:
    //==========================
    // Binary
    //==========================
    template <typename T>
    static std::vector<unsigned char> binary_serialize(const ptr_const<T>& obj)
    {
        serialization::multi_process_stream buffer;
        serialization::save<serialization::multi_process_stream, ptr_const<T>>(buffer, obj);
        return buffer.GetRawData();
    };

    template <typename T>
    static auto binary_deserialize(const std::vector<unsigned char>& buffer_ref)
    {
        serialization::multi_process_stream buffer;
        buffer.SetRawData(buffer_ref);
        ptr_const<T> ptr_t;
        serialization::load<serialization::multi_process_stream, ptr_const<T>>(buffer, ptr_t);
        return ptr_t;
    };

    SERIALIZATION_API static void write_binary(
        const std::string& fn, const std::vector<unsigned char>& buffer);

    SERIALIZATION_API static void read_binary(
        const std::string& fn, std::vector<unsigned char>& buffer);

    template <typename T>
    static void write_to_binary(const std::string& fn, const ptr_const<T>& obj)
    {
        const std::vector<unsigned char>& buffer = binary_serialize(obj);
        write_binary(fn, buffer);
    }

    template <typename T>
    static ptr_const<T> read_from_binary(const std::string& path)
    {
        std::vector<unsigned char> buffer;
        read_binary(path, buffer);
        return binary_deserialize<T>(buffer);
    }

    //==========================
    // Json
    //==========================
    template <typename T>
    static std::string print(const ptr_const<T>& obj)
    {
        json value;
        serialization::save(value, obj);

        return value.dump(2);
    };

    template <typename T>
    static void json_serialize(json& value, const ptr_const<T>& obj)
    {
        auto& data = value["root"];
        serialization::save(data, obj);
    };

    template <typename T>
    static void json_deserialize(const json& value, ptr_const<T>& obj)
    {
        auto& data = const_cast<json&>(value["root"]);
        serialization::load(data, obj);
    };

    SERIALIZATION_API static void read_json(const std::string& path, json& root);

    SERIALIZATION_API static void write_json(const std::string& path, const json& root);

    template <typename T>
    static auto read_from_json(const std::string& path)
    {
        //SERIALIZATION_CHECK(std::filesystem::exists(path), "File does not exist: " + path);
        json root;
        read_json(path, root);
        ptr_const<T> obj;
        json_deserialize(root, obj);
        return obj;
    }

    template <typename T>
    static void write_to_json(const std::string& path, const ptr_const<T>& obj)
    {
        json root;
        json_serialize(root, obj);
        write_json(path, root);
    }

    //==========================
    // XML
    //==========================
    template <typename T>
    static std::string print_xml(const ptr_const<T>& obj)
    {
        pugi::xml_document doc;
        auto root = doc.append_child("root");
        serialization::save(root, obj);

        std::ostringstream oss;
        doc.save(oss, "  ");
        return oss.str();
    };

    template <typename T>
    static void xml_serialize(pugi::xml_document& doc, const ptr_const<T>& obj)
    {
        auto root = doc.child("root");
        if (!root)
        {
            root = doc.append_child("root");
        }
        serialization::save(root, obj);
    };

    template <typename T>
    static void xml_deserialize(const pugi::xml_document& doc, ptr_const<T>& obj)
    {
        auto root = doc.child("root");
        if (root)
        {
            serialization::load(const_cast<pugi::xml_node&>(root), obj);
        }
    };

    SERIALIZATION_API static void read_xml(const std::string& path, pugi::xml_document& doc);

    SERIALIZATION_API static void write_xml(const std::string& path, const pugi::xml_document& doc);

    template <typename T>
    static auto read_from_xml(const std::string& path)
    {
        pugi::xml_document doc;
        read_xml(path, doc);
        ptr_const<T> obj;
        xml_deserialize(doc, obj);
        return obj;
    }

    template <typename T>
    static void write_to_xml(const std::string& path, const ptr_const<T>& obj)
    {
        pugi::xml_document doc;
        xml_serialize(doc, obj);
        write_xml(path, doc);
    }
};  // access
}  // namespace serialization_impl
}  // namespace serialization
