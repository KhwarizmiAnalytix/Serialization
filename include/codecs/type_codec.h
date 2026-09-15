#pragma once

namespace serialization
{
// Client specializations implement archive-independent save/load methods and
// recurse through the supplied context. The empty primary provides no fallback.
template<class T, class Enable = void> struct type_codec {};
}
