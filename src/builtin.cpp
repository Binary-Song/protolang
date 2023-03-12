

#include "builtin.h"

namespace protolang
{
uptr<BuiltInInt> BuiltInInt::s_instance =
    std::make_unique<BuiltInInt>();

uptr<BuiltInFloat> BuiltInFloat::s_instance =
    std::make_unique<BuiltInFloat>();

uptr<BuiltInDouble> BuiltInDouble::s_instance =
    std::make_unique<BuiltInDouble>();
} // namespace protolang