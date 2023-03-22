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
	std::filesystem::path   m_output_file;
	std::unique_ptr<Logger> m_logger;

public:
	Logger &logger() { return *m_logger; }

	Compiler(const std::string &input_file,
	         const std::string &output_file = "");

	void compile();
};

} // namespace protolang
