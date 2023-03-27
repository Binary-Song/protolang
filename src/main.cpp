#include "compiler.h"
#include "encoding.h"
#include "log.h"

int main(int argc, char **argv)
{
	using namespace protolang;

	if (argc <= 1)
	{
		std::cerr << "Usage: protolang <source>\n";
		return 1;
	}

	StringU8 input_file_name = to_u8(std::string(argv[1]));
	protolang::Compiler compiler(input_file_name);
	try
	{
		compiler.compile();
	}
	catch (const protolang::Error &e)
	{
		e.print(compiler.logger());
		return 1;
	}
	return 0;
}
