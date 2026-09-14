
/**
 * @class   multi_process_stream
 * @brief   stream used to pass data across processes
 * using parallel_multi_process_controller.
 *
 * multi_process_stream is used to pass data across processes.
 * Using multi_process_stream it is possible to send data whose
 * length is not known at the receiving end.
 *
 * @warning
 * Note, stream operators cannot be combined with the Push/Pop array operators.
 * For example, if you push an array to the stream,
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

#include "util/export.h"

namespace serialization
{
class SERIALIZATION_API multi_process_stream
{
public:
    multi_process_stream();
    multi_process_stream(const multi_process_stream& other);
    ~multi_process_stream();
    multi_process_stream& operator=(const multi_process_stream& other);

    //@{
    /**
     * Add-to-stream operators. Adds to the end of the stream.
     */
    multi_process_stream& operator<<(double value);
    multi_process_stream& operator<<(float value);
    multi_process_stream& operator<<(int value);
    multi_process_stream& operator<<(short value);
    multi_process_stream& operator<<(char value);
    multi_process_stream& operator<<(bool value);
    multi_process_stream& operator<<(unsigned int value);
    multi_process_stream& operator<<(unsigned char value);
    multi_process_stream& operator<<(int64_t value);
    //multi_process_stream& operator<<(uint64_t value);
    multi_process_stream& operator<<(size_t value);
    multi_process_stream& operator<<(const std::string& value);
    multi_process_stream& operator<<(const std::string_view& value);
    multi_process_stream& operator<<(const char* value);
    //@}

    //@{
    /**
     * Remove-from-stream operators. Removes from the head of the stream.
     */
    multi_process_stream& operator>>(double& value);
    multi_process_stream& operator>>(float& value);
    multi_process_stream& operator>>(int& value);
    multi_process_stream& operator>>(short& value);
    multi_process_stream& operator>>(char& value);
    multi_process_stream& operator>>(bool& value);
    multi_process_stream& operator>>(unsigned int& value);
    multi_process_stream& operator>>(unsigned char& value);
    multi_process_stream& operator>>(int64_t& value);
    //multi_process_stream& operator>>(uint64_t& value);
    multi_process_stream& operator>>(size_t& value);
    multi_process_stream& operator>>(std::string& value);
    multi_process_stream& operator>>(std::string_view& value);
    //@}

    //@{
    /**
     * Add-array-to-stream methods. Adds to the end of the stream
     */
    void Push(const double* array, unsigned int size);
    void Push(const float* array, unsigned int size);
    void Push(const int* array, unsigned int size);
    void Push(const char* array, unsigned int size);
    void Push(const unsigned int* array, unsigned int size);
    void Push(const unsigned char* array, unsigned int size);
    void Push(const int64_t* array, unsigned int size);
    //void Push(const uint64_t* array, unsigned int size);
    void Push(const size_t* array, unsigned int size);
    //@}

    //@{
    /**
     * Remove-array-to-stream methods. Removes from the head of the stream.
     * Note: If the input array is nullptr, the array will be allocated internally
     * and the calling application is responsible for properly de-allocating it.
     * If the input array is not nullptr, it is expected to match the size of the
     * data internally, and this method would just fill in the data.
     */
    void Pop(double*& array, unsigned int& size);
    void Pop(float*& array, unsigned int& size);
    void Pop(int*& array, unsigned int& size);
    void Pop(char*& array, unsigned int& size);
    void Pop(unsigned int*& array, unsigned int& size);
    void Pop(unsigned char*& array, unsigned int& size);
    void Pop(int64_t*& array, unsigned int& size);
    //void Pop(uint64_t*& array, unsigned int& size);
    void Pop(size_t*& array, unsigned int& size);
    //@}

    /**
     * Clears everything in the stream.
     */
    void Reset();

    /**
     * Returns the size of the stream.
     */
    int Size();

    /**
     * Returns the size of the raw data returned by GetRawData. This
     * includes 1 byte to store the endian type.
     */
    int RawSize();

    /**
     * Returns true iff the stream is empty.
     */
    bool Empty();

    //@{
    /**
     * Serialization methods used to save/restore the stream to/from raw data.
     * Note: The 1st byte of the raw data buffer consists of the endian type.
     */
    std::vector<unsigned char> GetRawData();
    void                       SetRawData(const std::vector<unsigned char>& data);
    //@}

    unsigned char endianness() const;

private:
    class serializationInternals
    {
    public:
        using DataType = std::deque<unsigned char>;
        DataType data_;

        enum Types
        {
            int32_value,
            uint32_value,
            char_value,
            uchar_value,
            double_value,
            float_value,
            string_value,
            int64_value,
            uint64_value,
            size_value
        };

        void Push(const unsigned char* data, size_t length)
        {
            for (size_t cc = 0; cc < length; cc++)
            {
                data_.push_back(data[cc]);
            }
        }

        void Pop(unsigned char* data, size_t length)
        {
            if (!data_.empty())
            {
                for (size_t cc = 0; cc < length; cc++)
                {
                    data[cc] = data_.front();
                    data_.pop_front();
                }
            }
        }
    };

    serializationInternals* internals_;
    unsigned char           endianness_;
    enum
    {
        BigEndian,
        LittleEndian
    };
};
}  // namespace serialization

// SERIALIZATION-HeaderTest-Exclude: multi_process_stream.h
