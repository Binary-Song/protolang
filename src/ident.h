#pragma once
#include <string>
#include "token.h"
namespace protolang
{

struct Ident
{
	StringU8 name;
	SrcRange range;

public:
	Ident() {}

	Ident(StringU8 name, const SrcRange &location)
	    : name(std::move(name))
	    , range(location)
	{}
	StringU8 dump_json() { return u8'"' + name + u8'"'; }
};
} // namespace protolang