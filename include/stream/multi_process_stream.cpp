#include "stream/multi_process_stream.h"

#include <cassert>
#include <deque>

namespace serialization
{
//----------------------------------------------------------------------------
multi_process_stream::multi_process_stream()
{
    internals_ = new multi_process_stream::serializationInternals();
#ifdef SERIALIZATION_WORDS_BIGENDIAN
    endianness_ = multi_process_stream::BigEndian;
#else
    endianness_ = multi_process_stream::LittleEndian;
#endif
}

//----------------------------------------------------------------------------
multi_process_stream::~multi_process_stream()
{
    delete internals_;
    internals_ = nullptr;
}

//----------------------------------------------------------------------------
multi_process_stream::multi_process_stream(const multi_process_stream& other)
{
    internals_        = new multi_process_stream::serializationInternals();
    internals_->data_ = other.internals_->data_;
    endianness_       = other.endianness_;
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator=(const multi_process_stream& other)
{
    if (&other != this)
    {
        internals_->data_ = other.internals_->data_;
        endianness_       = other.endianness_;
    }
    return (*this);
}

//----------------------------------------------------------------------------
void multi_process_stream::Reset()
{
    internals_->data_.clear();
}

//----------------------------------------------------------------------------
int multi_process_stream::Size()
{
    return (static_cast<int>(internals_->data_.size()));
}

//----------------------------------------------------------------------------
int multi_process_stream::RawSize()
{
    return (Size() + 1);
};
//----------------------------------------------------------------------------
bool multi_process_stream::Empty()
{
    return (internals_->data_.empty());
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const double* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::double_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(double) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const float* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::float_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(float) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const int* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::int32_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(int) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const char* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::char_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(char) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const unsigned int* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::uint32_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(unsigned int) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const unsigned char* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::uchar_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(array, size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const int64_t* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::int64_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(uint64_t) * size);
}

//----------------------------------------------------------------------------
//void multi_process_stream::Push(const uint64_t* array, unsigned int size)
//{
//    assert("pre: array is nullptr!" && (array != nullptr));
//    internals_->data_.push_back(serializationInternals::uint64_value);
//    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
//    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(uint64_t) * size);
//}

//----------------------------------------------------------------------------
void multi_process_stream::Push(const size_t* array, unsigned int size)
{
    assert("pre: array is nullptr!" && (array != nullptr));
    internals_->data_.push_back(serializationInternals::size_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
    internals_->Push(reinterpret_cast<const unsigned char*>(array), sizeof(size_t) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(double*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be double" &&
        internals_->data_.front() == serializationInternals::double_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate array
        array = new double[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(double) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(float*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be float" &&
        internals_->data_.front() == serializationInternals::float_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate array
        array = new float[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(float) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(int*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be int" &&
        internals_->data_.front() == serializationInternals::int32_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new int[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(int) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(char*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be of type char" &&
        internals_->data_.front() == serializationInternals::char_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new char[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(char) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(unsigned int*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be of type unsigned int" &&
        internals_->data_.front() == serializationInternals::uint32_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new unsigned int[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(unsigned int) * size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(unsigned char*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be of type unsigned char" &&
        internals_->data_.front() == serializationInternals::uchar_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new unsigned char[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(array, size);
}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(int64_t*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be of type int64_t" &&
        internals_->data_.front() == serializationInternals::int64_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new int64_t[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(int64_t) * size);
}

//----------------------------------------------------------------------------
//void multi_process_stream::Pop(uint64_t*& array, unsigned int& size)
//{
//    assert(
//        "pre: stream data must be of type uint64_t" &&
//        internals_->data_.front() == serializationInternals::uint64_value);
//    internals_->data_.pop_front();
//
//    if (array == nullptr)
//    {
//        // Get the size of the array
//        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));
//
//        // Allocate the array
//        array = new uint64_t[size];
//        assert("ERROR: cannot allocate array" && (array != nullptr));
//    }
//    else
//    {
//        unsigned int sz;
//
//        // Get the size of the array
//        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
//        assert("ERROR: input array size does not match size of data" && (sz == size));
//    }
//
//    // Pop the array data
//    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(uint64_t) * size);
//}

//----------------------------------------------------------------------------
void multi_process_stream::Pop(size_t*& array, unsigned int& size)
{
    assert(
        "pre: stream data must be of type size_t" &&
        internals_->data_.front() == serializationInternals::size_value);
    internals_->data_.pop_front();

    if (array == nullptr)
    {
        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&size), sizeof(unsigned int));

        // Allocate the array
        array = new size_t[size];
        assert("ERROR: cannot allocate array" && (array != nullptr));
    }
    else
    {
        unsigned int sz;

        // Get the size of the array
        internals_->Pop(reinterpret_cast<unsigned char*>(&sz), sizeof(unsigned int));
        assert("ERROR: input array size does not match size of data" && (sz == size));
    }

    // Pop the array data
    internals_->Pop(reinterpret_cast<unsigned char*>(array), sizeof(size_t) * size);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(double value)
{
    internals_->data_.push_back(serializationInternals::double_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(double));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(float value)
{
    internals_->data_.push_back(serializationInternals::float_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(float));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(int value)
{
    internals_->data_.push_back(serializationInternals::int32_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(int));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(short value)
{
    internals_->data_.push_back(serializationInternals::int32_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(short));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(char value)
{
    internals_->data_.push_back(serializationInternals::char_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(char));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(bool value)
{
    auto v = static_cast<char>(value);
    internals_->data_.push_back(serializationInternals::char_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&v), sizeof(char));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(unsigned int value)
{
    internals_->data_.push_back(serializationInternals::uint32_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(unsigned char value)
{
    internals_->data_.push_back(serializationInternals::uchar_value);
    internals_->Push(&value, sizeof(unsigned char));
    return (*this);
}

//-----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(int64_t value)
{
    internals_->data_.push_back(serializationInternals::int64_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(int64_t));
    return (*this);
}

//-----------------------------------------------------------------------------
//multi_process_stream& multi_process_stream::operator<<(uint64_t value)
//{
//    internals_->data_.push_back(serializationInternals::uint64_value);
//    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(uint64_t));
//    return (*this);
//}

//-----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(size_t value)
{
    internals_->data_.push_back(serializationInternals::size_value);
    internals_->Push(reinterpret_cast<unsigned char*>(&value), sizeof(size_t));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(const char* value)
{
    operator<<(std::string(value));
    return *this;
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(const std::string& value)
{
    // Find the real string size
    auto size = static_cast<int>(value.size());

    // Set the type
    internals_->data_.push_back(serializationInternals::string_value);

    // Set the string size
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(int));

    // Set the string content
    for (int idx = 0; idx < size; idx++)
    {
        internals_->Push(reinterpret_cast<const unsigned char*>(&value[idx]), sizeof(char));
    }
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator<<(const std::string_view& value)
{
    // Find the real string_view size
    auto size = static_cast<int>(value.size());

    // Set the type
    internals_->data_.push_back(serializationInternals::string_value);

    // Set the string_view size
    internals_->Push(reinterpret_cast<unsigned char*>(&size), sizeof(int));

    // Set the string_view content
    for (int idx = 0; idx < size; idx++)
    {
        internals_->Push(reinterpret_cast<const unsigned char*>(&value[idx]), sizeof(char));
    }
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(double& value)
{
    assert(internals_->data_.front() == serializationInternals::double_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(double));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(float& value)
{
    assert(internals_->data_.front() == serializationInternals::float_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(float));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(int& value)
{
    value = 0;
    assert(internals_->data_.front() == serializationInternals::int32_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(int));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(short& value)
{
    // Automatically convert 64 bit values in case we are trying to transfer
    // int64_t with processes compiled with 32/64 values.
    if (internals_->data_.front() == serializationInternals::int64_value)
    {
        int64_t value64;
        (*this) >> value64;
        value = static_cast<short>(value64);
        return (*this);
    }
    assert(internals_->data_.front() == serializationInternals::int32_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(short));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(char& value)
{
    assert(internals_->data_.front() == serializationInternals::char_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(char));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(bool& value)
{
    char v;
    assert(internals_->data_.front() == serializationInternals::char_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&v), sizeof(char));
    value = (v != 0);
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(unsigned int& value)
{
    assert(internals_->data_.front() == serializationInternals::uint32_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned int));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(unsigned char& value)
{
    assert(internals_->data_.front() == serializationInternals::uchar_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(unsigned char));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(int64_t& value)
{
    assert(internals_->data_.front() == serializationInternals::int64_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(int64_t));
    return (*this);
}

//----------------------------------------------------------------------------
//multi_process_stream& multi_process_stream::operator>>(uint64_t& value)
//{
//    assert(internals_->data_.front() == serializationInternals::uint64_value);
//    internals_->data_.pop_front();
//    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(uint64_t));
//    return (*this);
//}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(size_t& value)
{
    assert(internals_->data_.front() == serializationInternals::size_value);
    internals_->data_.pop_front();
    internals_->Pop(reinterpret_cast<unsigned char*>(&value), sizeof(size_t));
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(std::string& value)
{
    value = "";
    assert(internals_->data_.front() == serializationInternals::string_value);
    internals_->data_.pop_front();
    int stringSize;
    internals_->Pop(reinterpret_cast<unsigned char*>(&stringSize), sizeof(int));
    char c_value;
    for (int idx = 0; idx < stringSize; idx++)
    {
        internals_->Pop(reinterpret_cast<unsigned char*>(&c_value), sizeof(char));
        value += c_value;
    }
    return (*this);
}

//----------------------------------------------------------------------------
multi_process_stream& multi_process_stream::operator>>(std::string_view& value)
{
    assert(internals_->data_.front() == serializationInternals::string_value);
    internals_->data_.pop_front();
    int stringSize;
    internals_->Pop(reinterpret_cast<unsigned char*>(&stringSize), sizeof(int));
    char        c_value;
    std::string val = " ";
    for (int idx = 0; idx < stringSize; idx++)
    {
        internals_->Pop(reinterpret_cast<unsigned char*>(&c_value), sizeof(char));
        val += c_value;
    }

    value = static_cast<std::string_view>(val);
    value.remove_prefix(1);
    return (*this);
}

//----------------------------------------------------------------------------
std::vector<unsigned char> multi_process_stream::GetRawData()
{
    const unsigned int         size = Size() + 1;
    std::vector<unsigned char> ret(size + 1);
    int                        idx = 0;
    for (auto iter = internals_->data_.begin(); iter != internals_->data_.end(); ++iter, ++idx)
    {
        ret[idx] = *iter;
    }
    ret[idx] = endianness_;

    return ret;
}

//----------------------------------------------------------------------------
void multi_process_stream::SetRawData(const std::vector<unsigned char>& data)
{
    internals_->data_.clear();
    if (!data.empty())
    {
        const auto endianness = data.back();
        internals_->data_.resize(data.size() - 1);
        int cc = 0;
        for (; cc < static_cast<int>(data.size() - 1); cc++)
        {
            internals_->data_[cc] = data[cc];
        }
        if (endianness_ != endianness)
        {
            endianness_ = endianness;
        }
    }
}

//----------------------------------------------------------------------------
unsigned char multi_process_stream::endianness() const
{
    return endianness_;
};
}  // namespace serialization
