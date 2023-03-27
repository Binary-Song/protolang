#pragma once
#include "encoding.h"
#include "guessing.h"
#include "linker.h"
namespace protolang
{

class COFFLinker : public Linker
{
	EnvGuesser eg;

public:
	explicit COFFLinker();
	std::filesystem::path link(
	    const std::vector<std::filesystem::path> &inputs,
	    const std::filesystem::path &output) const override;
};
} // namespace protolang
