#include "compiler.h"
#include "encoding.h"
#include "log.h"

int main()
{
	using namespace protolang;
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
