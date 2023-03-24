#include "guessing.h"
#include "3rdparty/glob/glob.h"
#include "exceptions.h"
namespace protolang
{

StringU8 guess_linker_path()
{
	auto result = glob::glob("C:\\Program Files*\\Microsoft Visual "
	                         "Studio\\*\\Community\\VC\\Tools\\MSVC\\"
	                         "*\\bin\\Hostx64\\x64\\link.exe");
	if (!result.empty())
		return result.at(0).string();
	throw ExceptionLinkerNotFound();
}

} // namespace protolang