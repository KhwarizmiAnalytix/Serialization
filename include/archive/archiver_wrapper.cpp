#include "archive/archiver_wrapper.h"

#include "util/export.h"
#include "util/registry.h"

namespace serialization
{
//=============================================================================
// Global Registry Definitions
//=============================================================================

/// @brief Global registry for JSON serialization functions
/// Maps type names to their corresponding serialization callbacks
SERIALIZATION_API SERIALIZATION_DEFINE_FUNCTION_REGISTRY(
    JsonSerializationRegistry, json_serialization_function_t);

/// @brief Global registry for XML serialization functions
/// Maps type names to their corresponding serialization callbacks
SERIALIZATION_API SERIALIZATION_DEFINE_FUNCTION_REGISTRY(
    XmlSerializationRegistry, xml_serialization_function_t);

/// @brief Global registry for binary serialization functions
/// Maps type names to their corresponding serialization callbacks
SERIALIZATION_API SERIALIZATION_DEFINE_FUNCTION_REGISTRY(
    BinarySerializationRegistry, binary_serialization_function_t);

}  // namespace serialization
