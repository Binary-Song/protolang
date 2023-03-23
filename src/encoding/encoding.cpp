#include "encoding.h"
#ifdef _WIN32
#include "encoding/win/encoding_win.h"
#endif
#include <string>
namespace protolang
{
std::string to_narrow(const u8str &s)
{
#ifdef _WIN32
	return to_narrow_win(s);
#else
	return std::string{(const char *)s.data(), s.size()};
#endif
}

u8str to_u8str(const std::string &s)
{
#ifdef _WIN32
	return to_u8str_win(s);
#else
	return u8str{(const char8_t *)s.data(), s.size()};
#endif
}
} // namespace protolang