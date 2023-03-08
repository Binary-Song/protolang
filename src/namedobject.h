#pragma once
#include <string>
#include "typedef.h"
namespace protolang
{
class NamedObject
{
public:
	virtual ~NamedObject() = default;
};

class NamedVar : public NamedObject
{
public:
	std::string name;
	std::string type;

	NamedVar(std::string name, std::string type)
	    : name(std::move(name))
	    , type(std::move(type))
	{}
};

class NamedFunc : public NamedObject
{
public:
	std::string           name;
	std::string           return_type;
	std::vector<NamedVar> params;
};

} // namespace protolang