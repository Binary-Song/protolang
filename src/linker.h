#pragma once
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
	virtual ~Linker()                                  = default;
	virtual void link(const std::vector<u8str> &inputs,
	                  const u8str &output) const = 0;
};

std::unique_ptr<Linker> create_linker(LinkerType type);

} // namespace protolang
