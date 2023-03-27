#include "guessing.h"
#include "3rdparty/glob/glob.h"
#include "encoding.h"
#include "exceptions.h"
#include "log.h"
namespace protolang
{

EnvGuesser::EnvGuesser()
{
	auto result = glob::glob(
	    StringU8("C:\\Program Files*\\Microsoft Visual "
	             "Studio\\*\\*\\VC\\Tools\\MSVC\\*\\")
	        .to_native());
	if (!result.empty())
	{
		m_msvc = result.at(0);
	}
	else
	{
		ErrorCannotFindTool e;
		e.tool = "MSVC installation";
		throw e;
	}
}
std::filesystem::path EnvGuesser::guess_runtime_path() const
{
	auto result = glob::glob(StringU8{
	    m_msvc / StringU8(R"(lib\x64\msvcrt.lib)").to_path()}
	                             .to_native());
	if (!result.empty())
		return result.at(0);
	else
	{
		ErrorCannotFindTool e;
		e.tool = "msvcrt.lib";
		throw e;
	}
}

std::filesystem::path EnvGuesser::guess_linker_path() const
{
	auto result = glob::glob(StringU8{
	    m_msvc /
	    StringU8(R"(bin\Hostx64\x64\link.exe)").to_path()}
	                             .to_native());
	if (!result.empty())
		return result.at(0);
	else
	{
		ErrorCannotFindTool e;
		e.tool = "linker";
		throw e;
	}
}

} // namespace protolang