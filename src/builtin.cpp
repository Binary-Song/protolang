

#include "builtin.h"

namespace protolang
{
uptr<BuiltInInt> BuiltInInt::s_instance =
    std::make_unique<BuiltInInt>();

uptr<BuiltInFloat> BuiltInFloat::s_instance =
    std::make_unique<BuiltInFloat>();

} // namespace protolang