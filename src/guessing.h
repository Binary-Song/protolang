#pragma once
#include <filesystem>
#include <string>
namespace protolang
{
class EnvGuesser
{
	std::filesystem::path m_msvc;

public:
	EnvGuesser();
	std::filesystem::path guess_linker_path() const;
	std::filesystem::path guess_runtime_path() const;
};

} // namespace protolang