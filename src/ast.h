#pragma once
#include <format>
#include <functional>
#include <utility>
#include <vector>
#include "token.h"
#include "util.h"
namespace protolang
{
class Type;
class Env;
class NamedEntity;
struct NamedEntityProperties;
class TypeChecker;
struct Ast
{
public:
	Env *env;
	explicit Ast(Env *env)
	    : env(env)
	{}
	virtual ~Ast()                        = default;
	virtual std::string dump_json() const = 0;
};

struct TypeExpr : public Ast
{
public:
	explicit TypeExpr(Env *env)
	    : Ast(env)
	{}
};

struct IdentTypeExpr : public TypeExpr
{
	std::string name;
	explicit IdentTypeExpr(Env *env, std::string name)
	    : TypeExpr(env)
	    , name(std::move(name))
	{}
	virtual std::string dump_json() const override { return '"' + name + '"'; }
};

struct Expr : public Ast
{
public:
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	};

	ValueCat   value_cat = {};
	uptr<Type> type      = {};

	bool is_lvalue() const { return value_cat == ValueCat::Lvalue; }
	bool is_rvalue() const { return value_cat == ValueCat::Rvalue; }

	explicit Expr(Env *env);
	virtual ~Expr();
	virtual uptr<Type> get_type(TypeChecker *)
	{
		exit(1);
		return {};
	}
};

struct BinaryExpr : public Expr
{
public:
	uptr<Expr>  left;
	uptr<Expr>  right;
	std::string op;

public:
	BinaryExpr(Env *env, uptr<Expr> left, std::string op, uptr<Expr> right)
	    : Expr(env)
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

	virtual uptr<Type> get_type(TypeChecker *);
};

struct UnaryExpr : public Expr
{
public:
	bool        prefix;
	uptr<Expr>  right;
	std::string op;

public:
	UnaryExpr(Env *env, bool prefix, uptr<Expr> right, std::string op)
	    : Expr(env)
	    , prefix(prefix)
	    , right(std::move(right))
	    , op(std::move(op))
	{}

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "op": "{}", "right":{} }})", op, right->dump_json());
	}

	virtual uptr<Type> get_type(TypeChecker *);
};

struct CallExpr : public Expr
{
public:
	uptr<Expr>              callee;
	std::vector<uptr<Expr>> args;

public:
	CallExpr(Env *env, uptr<Expr> callee, std::vector<uptr<Expr>>)
	    : Expr(env)
	    , callee(std::move(callee))
	    , args(std::move(args))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "callee": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}

	virtual uptr<Type> get_type(TypeChecker *);
};

struct BracketExpr : public CallExpr
{
public:
	BracketExpr(Env *env, uptr<Expr> callee, std::vector<uptr<Expr>> args)
	    : CallExpr(env, std::move(callee), std::move(args))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "indexed": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}

	virtual uptr<Type> get_type(TypeChecker *);
};

/// 括号
struct GroupedExpr : public Expr
{
public:
	uptr<Expr> expr;

public:
	explicit GroupedExpr(Env *env, uptr<Expr> _expr)
	    : Expr(env)
	    , expr(std::move(_expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "grouped":{} }})", expr->dump_json());
	}
};

struct LiteralExpr : public Expr
{
public:
	Token token;

public:
	explicit LiteralExpr(Env *env, const Token &token)
	    : Expr(env)
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

struct IdentExpr : public Expr
{
public:
	std::string name;

public:
	explicit IdentExpr(Env *env, std::string name)
	    : Expr(env)
	    , name(std::move(name))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "id": "{}"  }})", name);
	}
};

struct Decl : public Ast
{
	std::string               name;
	virtual uptr<NamedEntity> declare(
	    const NamedEntityProperties &props) const = 0;

	Decl() = default;
	explicit Decl(std::string name)
	    : name(std::move(name))
	{}

	virtual void check_type(TypeChecker *) = 0;
};

struct VarDecl : public Decl
{
	uptr<TypeExpr> type;
	uptr<Expr>     init;

	VarDecl() = default;
	VarDecl(std::string name, uptr<TypeExpr> type, uptr<Expr> init)
	    : Decl(std::move(name))
	    , type(std::move(type))
	    , init(std::move(init))
	{}

	uptr<NamedEntity> declare(
	    const NamedEntityProperties &props) const override;

	std::string dump_json() const override
	{
		return std::format(R"({{ "name":"{}", "type": {} , "init":{} }})",
		                   name,
		                   type->dump_json(),
		                   init->dump_json());
	}

	void check_type(TypeChecker *) override;
};

struct ParamDecl : public Decl
{
	uptr<TypeExpr> type;

	ParamDecl() = default;
	ParamDecl(std::string name, uptr<TypeExpr> type)
	    : Decl(std::move(name))
	    , type(std::move(type))
	{}

	uptr<NamedEntity> declare(
	    const NamedEntityProperties &props) const override;

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "name":"{}", "type": {}  }})", name, type->dump_json());
	}

	void check_type(TypeChecker *) override {}
};

struct Stmt : public Ast
{
	Stmt(Env *env)
	    : Ast(env)
	{}
};

struct ExprStmt : public Stmt
{
	uptr<Expr> expr;

	explicit ExprStmt(uptr<Expr> expr)
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

	explicit CompoundStmtElem(uptr<VarDecl> var_decl)
	    : _var_decl(std::move(var_decl))
	{}

	Stmt    *stmt() const { return _stmt.get(); }
	VarDecl *var_decl() const { return _var_decl.get(); }

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
	uptr<VarDecl> _var_decl;
};

struct CompoundStmt : public Stmt
{
	uptr<Env>                           env;
	std::vector<uptr<CompoundStmtElem>> elems;

	CompoundStmt(uptr<Env> env);

	std::string dump_json() const override
	{
		return dump_json_for_vector_of_ptr(elems);
	}
};

struct FuncDecl : public Decl
{
	std::vector<uptr<ParamDecl>> params;
	uptr<TypeExpr>               return_type;
	uptr<CompoundStmt>           body;

	FuncDecl() = default;
	FuncDecl(std::string                  name,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	uptr<NamedEntity> declare(
	    const NamedEntityProperties &props) const override;

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "name":"{}", "return_type": {} , "body":{} }})",
		    name,
		    return_type->dump_json(),
		    body->dump_json());
	}
	void check_type(TypeChecker *) override {}
};

struct Program : public Ast
{
	std::vector<uptr<Decl>> decls;
	uptr<Env>               root_env;

	explicit Program(std::vector<uptr<Decl>> decls, uptr<Env> root_env);

	std::string dump_json() const
	{
		return std::format(R"({{ "decls":[ {} ] }})",
		                   dump_json_for_vector_of_ptr(decls));
	}
};

} // namespace protolang