#pragma once
#ifdef _WIN32
#include "encoding.h"
namespace protolang
{
std::string to_narrow_win(const u8str &);
u8str       to_u8str_win(const std::string &);
} // namespace protolang
#endif