#pragma once
#include <string>
#include <utility>
#include "ast.h"
#include "token.h"
#include "typedef.h"
namespace protolang
{

struct NamedEntityProperties
{
	Pos2DRange ident_pos;
	Pos2DRange available_pos;
};

enum class NamedEntityType
{
	NamedVar,
	NamedFunc,
};

class NamedEntity
{
public:
	using Properties = NamedEntityProperties;

	Properties  props;
	std::string name;

	NamedEntity(std::string name, const Properties &props)
	    : name(std::move(name))
	    , props(props)
	{}

	virtual ~NamedEntity()                            = default;
	virtual std::string     dump_json() const         = 0;
	virtual NamedEntityType named_entity_type() const = 0;
};

class NamedVar : public NamedEntity
{
public:
	TypeExpr *type;

	NamedVar(Properties props, std::string name, TypeExpr *type)
	    : NamedEntity(std::move(name), props)
	    , type(type)
	{}

	virtual std::string dump_json() const
	{
		return std::format(
		    R"({{ "name": "{}", "type": "{}" }})", name, type->dump_json());
	}

	NamedEntityType named_entity_type() const override
	{
		return NamedEntityType::NamedVar;
	}
};

class NamedFunc : public NamedEntity
{
public:
	TypeExpr             *return_type;
	std::vector<NamedVar> params;

	NamedFunc(const Properties            &props,
	          const std::string           &name,
	          TypeExpr                    *returnType,
	          const std::vector<NamedVar> &params)
	    : NamedEntity(name, props)
	    , return_type(returnType)
	    , params(params)
	{}

	virtual std::string dump_json() const
	{
		return std::format(
		    R"({{ "name": "{}", "return": "{}", "params": {} }})",
		    name,
		    return_type->dump_json(),
		    dump_json_for_vector(params));
	}

	NamedEntityType named_entity_type() const override
	{
		return NamedEntityType::NamedFunc;
	}
};

} // namespace protolang