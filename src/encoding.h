#pragma once
#include <string>
namespace protolang
{
using u8str = std::u8string;

std::string to_narrow(const u8str &);
u8str       to_u8str(const std::string &);

} // namespace protolang