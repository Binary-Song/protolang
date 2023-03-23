#ifdef _WIN32
#include "encoding/win/encoding_win.h"

#include <Windows.h>
namespace protolang
{
// Convert a wide Unicode string to an UTF8 string
static std::u8string utf8_encode(const std::wstring &wstr)
{
	int           size_needed = WideCharToMultiByte(CP_UTF8,
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

// Convert an UTF8 string to a wide Unicode String
static std::wstring utf8_decode(const std::u8string &str)
{
	int          size_needed = MultiByteToWideChar(CP_UTF8,
                                          0,
                                          (const char *)&str[0],
                                          (int)str.size(),
                                          NULL,
                                          0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8,
	                    0,
	                    (const char *)&str[0],
	                    (int)str.size(),
	                    &wstrTo[0],
	                    size_needed);
	return wstrTo;
}

// Convert an wide Unicode string to ANSI string
static std::string unicode2ansi(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(
	    CP_ACP, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
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
static std::wstring ansi2unicode(const std::string &str)
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

std::string to_narrow_win(const u8str &s)
{
	auto u16 = utf8_decode(s);
	auto uni = unicode2ansi(u16);
	return uni;
}

u8str to_u8str_win(const std::string &ansi)
{
	auto uni = ansi2unicode(ansi);
	auto u8  = utf8_encode(uni);
	return u8;
}

} // namespace protolang
#endif