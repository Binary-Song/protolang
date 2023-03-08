#pragma once
#include <format>
#include <functional>
#include <utility>
#include <vector>
#include "token.h"
#include "util.h"
namespace protolang
{

class Env;

struct Ast
{
public:
	virtual ~Ast()                        = default;
	virtual std::string dump_json() const = 0;
};

struct TypeExpr : public Ast
{
public:
	virtual std::string id() const = 0;
	bool equals(const TypeExpr &other) { return id() == other.id(); }
};

struct TypeExprIdent : public TypeExpr
{
	std::string name;
	explicit TypeExprIdent(std::string name)
	    : name(std::move(name))
	{}
	virtual std::string dump_json() const override { return '"' + name + '"'; }
	virtual std::string id() const override;
};

struct Expr : public Ast
{
public:
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	} value_cat;

	bool is_lvalue() const { return value_cat == ValueCat::Lvalue; }
	bool is_rvalue() const { return value_cat == ValueCat::Rvalue; }

	explicit Expr(ValueCat valueCat)
	    : value_cat(valueCat)
	{}
	virtual ~Expr() = default;
};

struct ExprBinary : public Expr
{
public:
	uptr<Expr>  left;
	uptr<Expr>  right;
	std::string op;

public:
	ExprBinary(ValueCat cat, uptr<Expr> left, std::string op, uptr<Expr> right)
	    : Expr(cat)
	    , left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "op":"{}", "left":{}, "right":{} }})",
		                   op,
		                   left->dump_json(),
		                   right->dump_json());
	}
};

struct ExprUnary : public Expr
{
public:
	bool        prefix;
	uptr<Expr>  right;
	std::string op;

public:
	ExprUnary(ValueCat cat, bool prefix, uptr<Expr> right, std::string op)
	    : Expr(cat)
	    , prefix(prefix)
	    , right(std::move(right))
	    , op(std::move(op))
	{}

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "op": "{}", "right":{} }})", op, right->dump_json());
	}
};

struct ExprCall : public Expr
{
public:
	uptr<Expr>              callee;
	std::vector<uptr<Expr>> args;

public:
	ExprCall(uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args,
	         ValueCat                cat = ValueCat::Rvalue)
	    : Expr(cat)
	    , callee(std::move(callee))
	    , args(std::move(args))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "callee": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}
};

struct ExprIndex : public ExprCall
{
public:
	ExprIndex(uptr<Expr>              callee,
	          std::vector<uptr<Expr>> args,
	          ValueCat                cat = ValueCat::Lvalue)
	    : ExprCall(std::move(callee), std::move(args), cat)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "indexed": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}
};

/// 括号
struct ExprGroup : public Expr
{
public:
	uptr<Expr> expr;

public:
	explicit ExprGroup(uptr<Expr> _expr)
	    : Expr(_expr->value_cat)
	    , expr(std::move(_expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "grouped":{} }})", expr->dump_json());
	}
};

struct ExprLiteral : public Expr
{
public:
	Token token;

public:
	explicit ExprLiteral(const Token &token)
	    : Expr(ValueCat::Rvalue)
	    , token(token)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "str":"{}", "int":{}, "fp":{} }})",
		                   token.str_data,
		                   token.int_data,
		                   token.fp_data);
	}
};

struct ExprIdent : public Expr
{
public:
	std::string name;

public:
	explicit ExprIdent(std::string name)
	    : Expr(ValueCat::Lvalue)
	    , name(std::move(name))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "id": "{}"  }})", name);
	}
};

class NamedObject;
struct NamedObjectProperties;

enum class DeclType
{
	Var,
	Param,
	Func,
};

struct Decl : public Ast
{
	std::string               name;
	virtual uptr<NamedObject> declare(
	    const NamedObjectProperties &props) const = 0;
	virtual DeclType decl_type() const            = 0;

	Decl() = default;
	explicit Decl(std::string name)
	    : name(std::move(name))
	{}
};

struct DeclVar : public Decl
{
	uptr<TypeExpr> type;
	uptr<Expr>     init;

	DeclVar() = default;
	DeclVar(std::string name, uptr<TypeExpr> type, uptr<Expr> init)
	    : Decl(std::move(name))
	    , type(std::move(type))
	    , init(std::move(init))
	{}

	uptr<NamedObject> declare(
	    const NamedObjectProperties &props) const override;

	DeclType decl_type() const override { return DeclType::Var; }

	std::string dump_json() const override
	{
		return std::format(R"({{ "name":"{}", "type": {} , "init":{} }})",
		                   name,
		                   type->dump_json(),
		                   init->dump_json());
	}
};

struct DeclParam : public Decl
{
	uptr<TypeExpr> type;

	DeclParam() = default;
	DeclParam(std::string name, uptr<TypeExpr> type)
	    : Decl(std::move(name))
	    , type(std::move(type))
	{}

	uptr<NamedObject> declare(
	    const NamedObjectProperties &props) const override;

	DeclType decl_type() const override { return DeclType::Param; }

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "name":"{}", "type": {}  }})", name, type->dump_json());
	}
};

struct Stmt : public Ast
{};

struct StmtExpr : public Stmt
{
	uptr<Expr> expr;

	explicit StmtExpr(uptr<Expr> expr)
	    : expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "expr":{} }})", expr->dump_json());
	}
};

struct CompoundStmtElem : public Ast
{

	explicit CompoundStmtElem(uptr<Stmt> stmt)
	    : _stmt(std::move(stmt))
	{}

	explicit CompoundStmtElem(uptr<DeclVar> var_decl)
	    : _var_decl(std::move(var_decl))
	{}

	Stmt    *stmt() const { return _stmt.get(); }
	DeclVar *var_decl() const { return _var_decl.get(); }

	std::string dump_json() const override
	{
		if (stmt())
			return stmt()->dump_json();
		if (var_decl())
			return var_decl()->dump_json();
		return "null";
	}

private:
	uptr<Stmt>    _stmt;
	uptr<DeclVar> _var_decl;
};

struct StmtCompound : public Stmt
{
	uptr<Env>                           env;
	std::vector<uptr<CompoundStmtElem>> elems;

	StmtCompound(uptr<Env> env);

	std::string dump_json() const override
	{
		return dump_json_for_vector_of_ptr(elems);
	}
};

struct DeclFunc : public Decl
{
	std::vector<uptr<DeclParam>> params;
	uptr<TypeExpr>               return_type;
	uptr<StmtCompound>           body;

	DeclFunc() = default;
	DeclFunc(std::string                  name,
	         std::vector<uptr<DeclParam>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<StmtCompound>           body);

	uptr<NamedObject> declare(
	    const NamedObjectProperties &props) const override;
	DeclType decl_type() const override { return DeclType::Func; }

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "name":"{}", "return_type": {} , "body":{} }})",
		    name,
		    return_type->dump_json(),
		    body->dump_json());
	}
};

struct Program : public Ast
{
	std::vector<uptr<Decl>> decls;

	explicit Program(std::vector<uptr<Decl>> decls)
	    : decls(std::move(decls))
	{}

	std::string dump_json() const
	{
		return std::format(R"({{ "decls":[ {} ] }})",
		                   dump_json_for_vector_of_ptr(decls));
	}
};

} // namespace protolang