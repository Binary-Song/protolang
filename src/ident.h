#pragma once
#include <string>
#include "token.h"
namespace protolang
{

struct Ident
{
	u8str name;
	SrcRange    range;

public:
	Ident() {}

	Ident(u8str name, const SrcRange &location)
	    : name(std::move(name))
	    , range(location)
	{}
	u8str dump_json() { return '"' + name + '"'; }
};
} // namespace protolang