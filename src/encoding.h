#pragma once
#include <iostream>
#include <string>
#include <string_view>
namespace protolang
{

#ifdef PROTOLANG_USE_WCHAR
using CharNative = wchar_t;
auto &&Cout      = std::wcout;
auto &&Cerr      = std::wcerr;
#else
using CharNative  = char;
auto       &&Cout = std::cout;
auto       &&Cerr = std::cerr;
#endif

using CharU8       = char8_t;
using StringNative = std::basic_string<CharNative>;
using StringU8     = std::basic_string<CharU8>;
using StringU8View = std::basic_string_view<CharU8>;

#ifdef PROTOLANG_HOST_MSVC
// 对Utf8字符串执行编码转换
// 参见 encoding_win.cpp
StringNative to_native(const StringU8 &);
StringU8     to_u8(const StringNative &);
#else
StringNative to_native(const StringU8 &s)
{
	return StringNative{(const CharNative *)s.data(), s.size()};
}
StringU8 to_u8(const StringNative &s)
{
	return StringU8{(const CharU8 *)s.data(), s.size()};
}
#endif

// 小心生命周期！！
StringU8View u8view(const std::string &s)
{
	return StringU8View{(const char8_t *)s.data(), s.size()};
}

StringU8View u8view(const char *s)
{
	return StringU8View{(const char8_t *)s};
}

StringU8View operator"" _u8view(const char *str, std::size_t len)
{
	return {(const char8_t *)str, len};
}

std::string_view sview(const std::u8string &s)
{
	return {(const char *)s.data(), s.size()};
}

std::string_view sview(const char8_t *s)
{
	return {(const char *)s};
}

// Native文件流



} // namespace protolang