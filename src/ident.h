#pragma once
#include <string>
#include "token.h"
namespace protolang
{

struct Ident
{
	std::string name;
	SrcRange    range;

public:
	Ident() {}

	Ident(std::string name, const SrcRange &location)
	    : name(std::move(name))
	    , range(location)
	{}
	std::string dump_json() const { return '"' + name + '"'; }
};
} // namespace protolang