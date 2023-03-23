#pragma once
#include "linker.h"
namespace protolang
{

class COFFLinker : public Linker
{
public:
	explicit COFFLinker();
	void link(const std::vector<std::string> &inputs,
	          const std::string &output) const override;
};

} // namespace protolang
