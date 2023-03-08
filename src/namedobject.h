#pragma once
#include <string>
#include <utility>
#include "ast.h"
#include "token.h"
#include "typedef.h"
namespace protolang
{

struct NamedObjectProperties
{
	Pos2DRange ident_pos;
	Pos2DRange available_pos;
};

class NamedObject
{
public:
	using Properties = NamedObjectProperties;

	Properties  props;
	std::string name;

	NamedObject(std::string name, const Properties &props)
	    : name(std::move(name))
	    , props(props)
	{}

	virtual ~NamedObject() = default;
	virtual std::string dump_json() const = 0;
};

class NamedVar : public NamedObject
{
public:
	TypeExpr *type;

	NamedVar(Properties props, std::string name, TypeExpr *type)
	    : NamedObject(std::move(name), props)
	    , type(type)
	{}

	virtual std::string dump_json() const
	{
		return std::format(R"({{ "name": "{}", "type": "{}" }})", name, type);
	}
};

class NamedFunc : public NamedObject
{
public:
	TypeExpr             *return_type;
	std::vector<NamedVar> params;

	NamedFunc(const Properties            &props,
	          const std::string           &name,
	          TypeExpr                    *returnType,
	          const std::vector<NamedVar> &params)
	    : NamedObject(name, props)
	    , return_type(returnType)
	    , params(params)
	{}

	virtual std::string dump_json() const
	{
		return std::format(
		    R"({{ "name": "{}", "return": "{}", "params": {} }})",
		    name,
		    return_type,
		    dump_json_for_vector(params));
	}
};

} // namespace protolang