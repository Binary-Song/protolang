#pragma once
#include <string>
#include <utility>
#include "ast.h"
#include "token.h"
#include "typedef.h"
#include "type.h"
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
	NamedType,
};

class NamedEntity
{
public:
	using Properties = NamedEntityProperties;

	Properties props;
	Ident      ident;

	NamedEntity(Ident ident, const Properties &props)
	    : ident(std::move(ident))
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

	NamedVar(Properties props, Ident ident, TypeExpr *type)
	    : NamedEntity(std::move(ident), props)
	    , type(type)
	{}

	virtual std::string dump_json() const
	{
		return std::format(R"({{ "ident": {} , "type": "{}" }})",
		                   ident.dump_json(),
		                   type->dump_json());
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
	          const Ident                 &ident,
	          TypeExpr                    *returnType,
	          const std::vector<NamedVar> &params)
	    : NamedEntity(ident, props)
	    , return_type(returnType)
	    , params(params)
	{}

	virtual std::string dump_json() const
	{
		return std::format(R"({{ "ident": {}, "return":  {} , "params": {} }})",
		                   ident.dump_json(),
		                   return_type->dump_json(),
		                   dump_json_for_vector(params));
	}

	NamedEntityType named_entity_type() const override
	{
		return NamedEntityType::NamedFunc;
	}
};

class NamedType : public NamedEntity
{
public:
	NamedType(const Properties &props, const Ident &ident)
	    : NamedEntity(ident, props)
	{}

	virtual std::string dump_json() const
	{
		return std::format(R"({{ "ident": {}  }})", ident.dump_json());
	}

	NamedEntityType named_entity_type() const override
	{
		return NamedEntityType::NamedType;
	}
};

} // namespace protolang