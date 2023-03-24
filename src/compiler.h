#pragma once
#include <filesystem>
#include <string>

namespace protolang
{
class Logger;

struct Compiler
{
private:
	std::filesystem::path   m_input_file;
	std::filesystem::path   m_output_file_no_ext;
	std::unique_ptr<Logger> m_logger;
	StringU8             m_linker_path;

public:
	Logger &logger() { return *m_logger; }

	Compiler(const StringU8 &input_file,
	         const StringU8 &output_file_no_ext = "");

	void compile();
};

} // namespace protolang
