#include "serialization.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iomanip>

#include "archive/archiver_wrapper.h"

namespace serialization::serialization_impl
{
void access::write_binary(const std::string& fn, const std::vector<unsigned char>& buffer)
{
    std::ofstream out(fn.c_str(), std::ios::binary | std::ios::ate);
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());  // NOLINT
}

void access::read_binary(const std::string& fn, std::vector<unsigned char>& buffer)
{
    std::ifstream in(fn.c_str(), std::ios::binary | std::ios::ate);

    //SERIALIZATION_CHECK(!in.fail(), "The file ", fn, " does not exist.");

    auto size = static_cast<size_t>(in.tellg());

    if (size == 0)
    {
        return;
    }

    buffer.resize(size);
    in.seekg(0);
    in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
}

void access::read_json(const std::string& path, json& root)
{
    std::ifstream str(path);
    str >> root;
}

void access::write_json(const std::string& path, const json& root)
{
    std::ofstream str(path);
    str << std::setw(1) << root << std::endl;
}

void access::read_xml(const std::string& path, pugi::xml_document& doc)
{
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    SERIALIZATION_CHECK(result, "XML parsing error in {}: {}", path, result.description());
}

void access::write_xml(const std::string& path, const pugi::xml_document& doc)
{
    doc.save_file(path.c_str(), "  ");
}
}  // namespace serialization::serialization_impl
