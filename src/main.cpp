#include "compiler.h"
#include "log.h"
namespace protolang
{} // namespace protolang

int main()
{
	StringU8 input_file_name =
	    __FILE__ R"(\..\..\test\test3.ptl)";
	protolang::Compiler compiler(input_file_name);
	try
	{
		compiler.compile();
	}
	catch (const protolang::Error &e)
	{
		e.print(compiler.logger());
	}
}
