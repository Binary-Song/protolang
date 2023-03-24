#pragma once
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
namespace protolang
{
// using StringU8     = std::basic_string<char8_t>;
using StringU8View = std::basic_string_view<char8_t>;

struct StringU8 : std::u8string
{
	using std::u8string::u8string;

	// 因为源代码字符集是UTF-8，所以这个成立
	template <unsigned Size>
	StringU8(const char (&literal)[Size])
	    : std::u8string((const char8_t *)literal, Size)
	{}

	// 本类可以和u8string随便转，但不能和
	// string 随便转
	StringU8(const std::u8string &s)
	    : std::u8string(s)
	{}

	StringU8(std::u8string &&s) noexcept
	    : std::u8string(std::move(s))
	{}

	operator std::u8string() const & { return {*this}; }
	operator std::u8string() && { return {std::move(*this)}; }

	std::string           as_str() const;
	std::string           to_native() const;
	std::filesystem::path to_path() const;
};

// MSVC需要对Utf8字符串执行编码转换
// 参见 encoding_win.cpp
#ifdef PROTOLANG_HOST_MSVC
std::string to_native(const StringU8 &);
StringU8    to_u8(const std::string &);
#else
inline std::string to_native(const StringU8 &s)
{
	return {(const char *)s.data(), s.size()};
}
inline StringU8 to_u8(const std::string &s)
{
	return {(const char8_t *)s.data(), s.size()};
}
inline std::filesystem::path StringU8::to_path() const
{
	return {this.as_str()};
}
#endif

inline StringU8 as_u8(const std::string &s)
{
	return {(const char8_t *)s.data(), s.size()};
}
inline std::string as_str(const StringU8 &s)
{
	return {(const char *)s.data(), s.size()};
}
inline std::string StringU8::as_str() const
{
	return protolang::as_str(*this);
}
inline std::string StringU8::to_native() const
{
	return protolang::to_native(*this);
}

inline StringU8View operator"" _u8view(const char *str,
                                       std::size_t len)
{
	return StringU8View{(const char8_t *)str, len};
}

} // namespace protolang