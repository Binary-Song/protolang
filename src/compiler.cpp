#include <fstream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <string>
#include <type_traits>
#include "compiler.h"
#include "code_generator.h"
#include "lexer.h"
#include "linker.h"
#include "log.h"
#include "logger.h"
#include "parser.h"
#include "scope.h"
#include "source_code.h"
namespace protolang
{
Compiler::Compiler(const StringU8 &input_file,
                   const StringU8 &output_file_no_ext)
    : m_input_path(input_file.to_path())
    , m_output_path_no_ext(output_file_no_ext.to_path())
{
	namespace fs = std::filesystem;
	m_input_path = (m_input_path);
	if (output_file_no_ext.empty())
		m_output_path_no_ext = m_input_path.stem();
}
void Compiler::compile()
{
	// 读取源代码
	std::ifstream input_stream(m_input_path);
	SourceCode    src;
	bool          read_result = src.read(input_stream);
	m_logger       = std::make_unique<Logger>(src, std::cerr);
	Logger &logger = *m_logger;
	if (!read_result)
	{
		ErrorRead e;
		e.path = m_input_path.u8string();
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
	auto   root_scope = protolang::Scope::create_root(logger);
	Parser parser(logger, std::move(tokens), root_scope.get());
	auto   program = parser.parse();
	// 中间代码生成
	CodeGenerator g(logger, StringU8{m_input_path.filename()});
	bool          success = false;
	program->validate(success);
	if (!success)
		return;
	program->codegen(g, success);
	g.module().print(llvm::outs(), nullptr);
	if (!success)
		return;
	// 目标代码生成
	auto obj_path = m_output_path_no_ext;
	obj_path += ".o";
	g.gen(obj_path);
	std::cout << StringU8(obj_path).to_native() << "\n\n";
	// 链接
	auto linker = create_linker(LinkerType::COFF);

	auto exe_path =
	    linker->link({obj_path}, m_output_path_no_ext.stem());
	std::cout << StringU8(exe_path).to_native() << std::endl;
}
} // namespace protolang