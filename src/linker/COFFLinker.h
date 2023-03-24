#pragma once
#include "encoding.h"
#include "linker.h"

namespace protolang
{

class COFFLinker : public Linker
{
public:
	explicit COFFLinker();
	void link(const std::vector<StringU8> &inputs,
	          const StringU8              &output) const override;
};
} // namespace protolang
