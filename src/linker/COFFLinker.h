#pragma once
#include "encoding.h"
#include "linker.h"

namespace protolang
{

class COFFLinker : public Linker
{
public:
	explicit COFFLinker();
	void link(const std::vector<u8str> &inputs,
	          const u8str              &output) const override;
};
} // namespace protolang
