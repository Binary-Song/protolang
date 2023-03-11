//#pragma once
//#include <string>
//#include <utility>
//#include "ast.h"
//#include "token.h"
//#include "type.h"
//#include "typedef.h"
//namespace protolang
//{
//
//struct NamedEntityProperties
//{
//	FileRange ident_pos;
//	FileRange available_pos;
//};
//
//enum class NamedEntityType
//{
//	NamedVar,
//	NamedFunc,
//	NamedType,
//};
//
//inline const char *to_string(NamedEntityType t)
//{
//	switch (t)
//	{
//	case NamedEntityType::NamedVar:
//		return "variable";
//	case NamedEntityType::NamedFunc:
//		return "function";
//	case NamedEntityType::NamedType:
//		return "type";
//	}
//	assert(false); // 没考虑到
//	return "";
//}
//
//class NamedEntity
//{
//public:
//	using Properties = NamedEntityProperties;
//
//	Properties props;
//	Ident      ident;
//
//	NamedEntity(Ident ident, const Properties &props)
//	    : ident(std::move(ident))
//	    , props(props)
//	{}
//
//	virtual ~NamedEntity()                            = default;
//	virtual std::string     dump_json() const         = 0;
//	virtual NamedEntityType named_entity_type() const = 0;
//};
//
//class NamedVar : public NamedEntity, public ITypeCached
//{
//	// 数据
//private:
//	ITyped     *type_ast;
//	uptr<IType> cached_type;
//
//	// 函数
//public:
//	static constexpr NamedEntityType static_type()
//	{
//		return NamedEntityType::NamedVar;
//	}
//
//	NamedVar(Properties props, Ident ident, ITyped *type_ast)
//	    : NamedEntity(std::move(ident), props)
//	    , type_ast(type_ast)
//	{}
//
//	std::string dump_json() const override
//	{
//		return std::format(R"({{ "ident": {} }})",
//		                   ident.dump_json());
//	}
//
//	NamedEntityType named_entity_type() const override
//	{
//		return static_type();
//	}
//
//private:
//	// ITyped
//	uptr<IType> &get_cached_type() override
//	{
//		return cached_type;
//	}
//	uptr<IType> solve_type(TypeChecker *tc) const override
//	{
//		return type_ast->get_type(tc)->clone();
//	}
//};
//
//class NamedFunc : public NamedEntity, public IFunc
//{
//	// 数据
//private:
//	ITyped                 *return_type;
//	std::vector<IVar *>     params;
//	IFuncBody              *body;
//
//	// 函数
//public:
//	NamedFunc(const Properties       &props,
//	          const Ident            &ident,
//	          ITyped                 *returnType,
//	          std::vector<NamedVar *> params,
//	          CompoundStmt           *body)
//	    : NamedEntity(ident, props)
//	    , return_type(returnType)
//	    , params(std::move(params))
//	    , body(body)
//	{}
//
//	virtual std::string dump_json() const
//	{
//		return std::format(R"({{ "ident": {}, "params": {} }})",
//		                   ident.dump_json(),
//		                   dump_json_for_vector_of_ptr(params));
//	}
//
//	NamedEntityType named_entity_type() const override
//	{
//		return static_type();
//	}
//
//	static constexpr NamedEntityType static_type()
//	{
//		return NamedEntityType::NamedFunc;
//	}
//
//private:
//	uptr<IType>  cached_type;
//	// ITyped
//	uptr<IType> &get_cached_type() override
//	{
//		return cached_type;
//	}
//	uptr<IType> solve_type(TypeChecker *tc) const override;
//};
//
//class NamedType : public NamedEntity, public IType
//{
//public:
//	std::vector<NamedEntity *> members;
//
//	NamedType(const Properties          &props,
//	          const Ident               &ident,
//	          std::vector<NamedEntity *> members)
//	    : NamedEntity(ident, props)
//	    , members(std::move(members))
//	{}
//
//	std::string dump_json() const override
//	{
//		return std::format(R"({{ "ident": {} }})",
//		                   ident.dump_json());
//	}
//
//	NamedEntityType named_entity_type() const override
//	{
//		return static_type();
//	}
//
//	static constexpr NamedEntityType static_type()
//	{
//		return NamedEntityType::NamedType;
//	}
//};
//
//} // namespace protolang