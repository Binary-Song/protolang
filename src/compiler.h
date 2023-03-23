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
	u8str             m_linker_path;

public:
	Logger &logger() { return *m_logger; }

	Compiler(const u8str &input_file,
	         const u8str &output_file_no_ext = "");

	void compile();
};

} // namespace protolang
