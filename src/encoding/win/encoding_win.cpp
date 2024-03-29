#ifdef PROTOLANG_HOST_MSVC
#include <string>
#include "encoding_win.h"
#include "encoding.h"

// WINDOWS include 放在后面
#include <Windows.h>

namespace protolang
{
// Convert a wide Unicode string to an UTF8 string
std::u8string wide2u8(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8,
	                                      0,
	                                      &wstr[0],
	                                      (int)wstr.size(),
	                                      NULL,
	                                      0,
	                                      NULL,
	                                      NULL);

	std::u8string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8,
	                    0,
	                    &wstr[0],
	                    (int)wstr.size(),
	                    (char *)(&strTo[0]),
	                    size_needed,
	                    NULL,
	                    NULL);
	return strTo;
}

std::wstring u8towide(const std::u8string &str)
{
	int          size_needed = MultiByteToWideChar(CP_UTF8,
                                          0,
                                          (const char *)&str[0],
                                          (int)str.size(),
                                          NULL,
                                          0);
	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8,
	                    0,
	                    (const char *)&str[0],
	                    (int)str.size(),
	                    &result[0],
	                    size_needed);
	return result;
}

// Convert an wide Unicode string to ANSI string
std::string wide2native(const std::wstring &wstr)
{
	int         size_needed = WideCharToMultiByte(CP_ACP,
                                          0,
                                          &wstr[0],
                                          (int)wstr.size(),
                                          NULL,
                                          0,
                                          NULL,
                                          NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP,
	                    0,
	                    &wstr[0],
	                    (int)wstr.size(),
	                    &strTo[0],
	                    size_needed,
	                    NULL,
	                    NULL);
	return strTo;
}

// Convert an ANSI string to a wide Unicode String
std::wstring native2wide(const std::string &str)
{
	int size_needed = MultiByteToWideChar(
	    CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP,
	                    0,
	                    &str[0],
	                    (int)str.size(),
	                    &wstrTo[0],
	                    size_needed);
	return wstrTo;
}

std::string to_native(const StringU8 &s)
{
	auto u16  = u8towide(s);
	auto ansi = wide2native(u16);
	return ansi;
}

StringU8 to_u8(const std::string &s)
{
	auto u16 = native2wide(s);
	auto u8  = wide2u8(u16);
	return u8;
}

std::filesystem::path StringU8::to_path() const
{
	return {u8towide(*this)};
}

// from path
StringU8::StringU8(const std::filesystem::path &path)
{
	*this = wide2u8(path.wstring());
}

} // namespace protolang
#endif