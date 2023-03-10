#pragma once
#include <string>
#include <utility>
#include "ast.h"
#include "token.h"
#include "type.h"
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
	NamedType,
};

inline const char *to_string(NamedEntityType t)
{
	switch (t)
	{
	case NamedEntityType::NamedVar:
		return "variable";
	case NamedEntityType::NamedFunc:
		return "function";
	case NamedEntityType::NamedType:
		return "type";
	}
	assert(false); // 没考虑到
	return "";
}

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

class NamedVar : public NamedEntity, public ITyped
{
	// 数据
private:
	TypeExpr  *type_ast;
	uptr<Type> cached_type;

	// 函数
public:
	static constexpr NamedEntityType static_type()
	{
		return NamedEntityType::NamedVar;
	}

	NamedVar(Properties props, Ident ident, TypeExpr *type_ast)
	    : NamedEntity(std::move(ident), props)
	    , type_ast(type_ast)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident": {} , "type": {} }})",
		                   ident.dump_json(),
		                   type_ast->dump_json());
	}

	NamedEntityType named_entity_type() const override { return static_type(); }

private:
	// ITyped
	uptr<Type> &get_cached_type() override { return cached_type; }
	uptr<Type>  solve_type(TypeChecker *tc) const override
	{
		return type_ast->get_type(tc)->clone();
	}
};

class NamedFunc : public NamedEntity
{
public:
	TypeExpr               *return_type;
	std::vector<NamedVar *> params;

	NamedFunc(const Properties       &props,
	          const Ident            &ident,
	          TypeExpr               *returnType,
	          std::vector<NamedVar *> params)
	    : NamedEntity(ident, props)
	    , return_type(returnType)
	    , params(std::move(params))
	{}

	FuncType *type(TypeChecker *tc)
	{
		if (!cached_type)
		{
			cached_type = solve_type(tc);
			assert(cached_type);
		}
		return cached_type.get();
	}

	virtual std::string dump_json() const
	{
		return std::format(R"({{ "ident": {}, "return":  {} , "params": {} }})",
		                   ident.dump_json(),
		                   return_type->dump_json(),
		                   dump_json_for_vector_of_ptr(params));
	}

	NamedEntityType named_entity_type() const override { return static_type(); }

	static constexpr NamedEntityType static_type()
	{
		return NamedEntityType::NamedFunc;
	}

private:
	uptr<FuncType> cached_type;

private:
	uptr<FuncType> solve_type(TypeChecker *tc) const;
};

class NamedType : public NamedEntity
{
public:
	std::vector<NamedEntity *> members;

	NamedType(const Properties          &props,
	          const Ident               &ident,
	          std::vector<NamedEntity *> members)
	    : NamedEntity(ident, props)
	    , members(std::move(members))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident": {} }})", ident.dump_json());
	}

	NamedEntityType named_entity_type() const override { return static_type(); }

	static constexpr NamedEntityType static_type()
	{
		return NamedEntityType::NamedType;
	}
};

} // namespace protolang