#pragma once
#include <string>
#include "token.h"
namespace protolang
{

struct Ident
{
	std::string name;
	Pos2DRange  location;

public:
	Ident() {}

	Ident(std::string name, const Pos2DRange &location)
	    : name(std::move(name))
	    , location(location)
	{}
	std::string dump_json() const { return '"' + name + '"'; }
};
} // namespace protolang