#include <fstream>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <type_traits>
#include "code_generator.h"
#include "env.h"
#include "lexer.h"
#include "logger.h"
#include "parser.h"
#include "source_code.h"
#include "token.h"
int main()
{
	try
	{
		std::string file_name =
		    __FILE__ R"(\..\..\test\test3.ptl)";
		std::string output_file_name =
		    __FILE__ R"(\..\..\dump\dump.json)";

		std::ifstream         f(file_name);
		bool                  file_good = f.good();
		protolang::SourceCode src(f);
		protolang::Logger     logger(src, std::cout);
		protolang::Lexer      lexer(src, logger);

		if (!file_good)
			logger.log(
			    protolang::FatalFileError(file_name, 'r'));

		std::vector<protolang::Token> tokens = lexer.scan();
		if (tokens.empty())
			return 1;

		auto root_env = protolang::Env::create_root(logger);
		protolang::Parser parser(
		    logger, std::move(tokens), root_env.get());
		auto prog = parser.parse();
		if (!prog)
			return 1;
		protolang::CodeGenerator g{"test"};
		try
		{
			prog->validate_types();
			root_env->codegen_all_funcs(g);
			prog->codegen(g);
			g.module().print(llvm::errs(), nullptr);
		}
		catch (const protolang::ExceptionPanic &)
		{
			return 1;
		}
	}
	catch (protolang::ExceptionFatalError error)
	{
		return 1;
	}
}
