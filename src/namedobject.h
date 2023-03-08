#pragma once
#include <string>
#include "typedef.h"
namespace protolang
{
class NamedObject
{
public:
	std::string name;

	struct Properties
	{
		Pos2DRange ident_pos;
		Pos2DRange available_pos;
	} props;

	NamedObject(const  std::string &name, const Properties &props)
	    : name(name)
	    , props(props)
	{}

	virtual ~NamedObject() = default;
};

class NamedVar : public NamedObject
{
public:
	std::string type;

	NamedVar(Properties props, std::string name, std::string type)
	    : NamedObject(std::move(name), props)
	    , type(std::move(type))
	{}
};

class NamedFunc : public NamedObject
{
public:
	std::string           return_type;
	std::vector<NamedVar> params;

	NamedFunc(const Properties &props,
	          const  std::string           &name,
	          const  std::string           &returnType,
	          const  std::vector<NamedVar> &params)
	    : NamedObject(name, props)
	    , return_type(returnType)
	    , params(params)
	{}
};

} // namespace protolang