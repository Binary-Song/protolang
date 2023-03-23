#include <fstream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <string>
#include <type_traits>
#include "compiler.h"
#include "code_generator.h"
#include "env.h"
#include "lexer.h"
#include "linker.h"
#include "log.h"
#include "logger.h"
#include "parser.h"
#include "source_code.h"
namespace protolang
{
Compiler::Compiler(const u8str &input_file,
                   const u8str &output_file_no_ext)
    : m_input_file(input_file)
    , m_output_file_no_ext(output_file_no_ext)
{
	namespace fs = std::filesystem;
	m_input_file = (m_input_file);
	if (output_file_no_ext == "")
	{
		m_output_file_no_ext = m_input_file.stem().string();
	}
	m_output_file_no_ext = (m_output_file_no_ext).string();
}
void Compiler::compile()
{
	// 读取源代码
	std::ifstream input_stream(m_input_file);
	SourceCode    src;
	bool          read_result = src.read(input_stream);
	m_logger       = std::make_unique<Logger>(src, std::cerr);
	Logger &logger = *m_logger;
	if (!read_result)
	{
		ErrorRead e;
		e.path = m_input_file.string();
		e.print(logger);
		return;
	}
	// 词法分析
	Lexer lexer(src, logger);
	auto  tokens = lexer.scan();
	if (tokens.empty())
	{
		ErrorEmptyInput e;
		throw std::move(e);
	}
	// 语法分析
	auto   root_env = protolang::Env::create_root(logger);
	Parser parser(logger, std::move(tokens), root_env.get());
	auto   program = parser.parse();
	// 中间代码生成
	CodeGenerator g(logger, m_input_file.filename().string());
	bool          success = false;
	program->validate_types(success);
	if (!success)
		return;
	program->codegen(g, success);
	if (!success)
		return;
	g.module().print(llvm::outs(), nullptr);
	// 目标代码生成
	u8str output_obj_file =
	    m_output_file_no_ext.string() + ".o";
	g.gen(output_obj_file);
	std::cout << output_obj_file << std::endl;

	// 狠狠地链接
	auto linker = create_linker(LinkerType::COFF);
	linker->link({output_obj_file},
	             m_output_file_no_ext.stem().string());

	u8str              output;
	llvm::raw_string_ostream out{output};
	llvm::raw_string_ostream err{output};
	std::cerr << output;
}
} // namespace protolang