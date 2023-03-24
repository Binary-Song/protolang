#pragma once
#include <string>
namespace protolang
{
std::u8string wide2u8(const std::wstring &wstr);
std::wstring  u82wide(const std::u8string &str);
std::string   wide2native(const std::wstring &wstr);
std::wstring  native2wide(const std::string &str);
} // namespace protolang