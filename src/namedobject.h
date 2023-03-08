#pragma once
#include <string>
#include <utility>
#include "typedef.h"
#include "ast.h"
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

	NamedObject(std::string name, const Properties &props)
	    : name(std::move(name))
	    , props(props)
	{}

	virtual ~NamedObject() = default;
};

class NamedVar : public NamedObject
{
public:
	TypeExpr *type;

	NamedVar(Properties props, std::string name, TypeExpr *type)
	    : NamedObject(std::move(name), props)
	    , type(std::move(type))
	{}
};

class NamedFunc : public NamedObject
{
public:
	std::string           return_type;
	std::vector<NamedVar> params;

	NamedFunc(const Properties            &props,
	          const std::string           &name,
	          std::string                  returnType,
	          const std::vector<NamedVar> &params)
	    : NamedObject(name, props)
	    , return_type(std::move(returnType))
	    , params(params)
	{}
};

} // namespace protolang