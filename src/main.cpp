#include <filesystem>
#include <fstream>
#include <type_traits>
#include "code_generator.h"
#include "env.h"
#include "lexer.h"
#include "log.h"
#include "logger.h"
#include "parser.h"
#include "source_code.h"
#include "token.h"
namespace protolang
{
struct Compiler
{
private:
	std::filesystem::path   m_input_file;
	std::filesystem::path   m_output_file;
	std::unique_ptr<Logger> m_logger;

public:
	Logger &logger() { return *m_logger; }

	Compiler(const std::string &input_file,
	         const std::string &output_file = "")
	    : m_input_file(input_file)
	    , m_output_file(output_file)
	{
		namespace fs = std::filesystem;
		m_input_file = (m_input_file);
		if (output_file == "")
		{
			m_output_file = m_input_file.filename();
		}
		m_output_file = (m_output_file);
	}

	void compile()
	{
		// 读取源代码
		std::ifstream input_stream(m_input_file);
		SourceCode    src;
		bool          read_result = src.read(input_stream);
		m_logger = std::make_unique<Logger>(src, std::cerr);
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
		CodeGenerator g(logger,
		                m_input_file.filename().string());
		bool          success = false;
		program->validate_types(success);
		if (!success)
			return;
		program->codegen(g, success);
		if (!success)
			return;
		//	g.module().print(llvm::outs(), nullptr);
		// 目标代码生成
		g.gen(this->m_output_file.string());
	}
};
} // namespace protolang

int main()
{
	std::string input_file_name =
	    __FILE__ R"(\..\..\test\test3.ptl)";
	protolang::Compiler compiler(input_file_name);
	try
	{
		compiler.compile();
	}
	catch (protolang::Error &e)
	{
		e.print(compiler.logger());
	}
}
