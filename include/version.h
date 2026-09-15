#pragma once

namespace serialization
{
// Defined out-of-line in version.cpp: the one translation unit compiled into
// libSerialization.* rather than inlined into every consumer.
const char* version() noexcept;
}
