#include "guessing.h"
#include "3rdparty/glob/glob.h"
#include "encoding.h"
#include "exceptions.h"
namespace protolang
{

std::filesystem::path guess_linker_path()
{
	auto result = glob::glob(
	    StringU8("C:\\Program Files*\\Microsoft Visual "
	             "Studio\\*\\Community\\VC\\Tools\\MSVC\\"
	             "*\\bin\\Hostx64\\x64\\link.exe")
	        .to_native());
	if (!result.empty())
		return result.at(0);
	throw ExceptionLinkerNotFound();
}

} // namespace protolang