#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "encoding.h"
namespace protolang
{

enum class LinkerType
{
	COFF
};

class Linker
{

public:
	explicit Linker();
	virtual ~Linker() = default;
	virtual std::filesystem::path link(
	    const std::vector<std::filesystem::path> &inputs,
	    const std::filesystem::path &output_no_ext) const = 0;
};

std::unique_ptr<Linker> create_linker(LinkerType type);

} // namespace protolang
