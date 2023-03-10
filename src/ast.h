#pragma once
#include <cassert>
#include <format>
#include <functional>
#include <utility>
#include <vector>
#include "ident.h"
#include "token.h"
#include "type.h"
#include "util.h"
namespace protolang
{
struct Type;
class Env;
class NamedEntity;
struct NamedEntityProperties;
class TypeChecker;

struct Ast
{
public:
	Env       *env;
	Pos2DRange range;

public:
	explicit Ast(Env *env, Pos2DRange range)
	    : env(env)
	    , range(std::move(range))
	{}

	virtual ~Ast() = default;
	virtual void check_type(TypeChecker *)
	{
		/* todo: 实现 */
		assert("Not implemented" && false);
	}
	virtual std::string dump_json() const = 0;
};

struct TypeExpr : public Ast, public ITypeCached
{
	// 数据
private:
	uptr<Type> cached_type;

	// 函数
public:
	explicit TypeExpr(Env *env, Pos2DRange range)
	    : Ast(env, range)
	{}

private:
	void check_type(TypeChecker *tc) override { this->get_type(tc); }

	// ITyped
	uptr<Type> &get_cached_type() override { return cached_type; }
};

struct IdentTypeExpr : public TypeExpr
{
	// 数据
public:
	Ident ident;

	// 函数
public:
	explicit IdentTypeExpr(Env *env, Ident ident)
	    : TypeExpr(env, ident.location)
	    , ident(std::move(ident))
	{}
	std::string dump_json() const override
	{
		return std::format(R"({{ "ident":  {}   }})", ident.dump_json());
	}

private:
	// ITyped
	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct Expr : public Ast, public ITypeCached
{
	// 类
public:
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	};

	// 数据
public:
	ValueCat value_cat = ValueCat::Pending;

private:
	uptr<Type> cached_type;

	// 函数
public:
	explicit Expr(Env *env, Pos2DRange range);
	virtual ~Expr();

	bool is_lvalue() const { return value_cat == ValueCat::Lvalue; }
	bool is_rvalue() const { return value_cat == ValueCat::Rvalue; }

	void check_type(TypeChecker *tc) override { get_type(tc); }

private:
	// ITyped
	uptr<Type> &get_cached_type() override { return cached_type; }
};

struct BinaryExpr : public Expr
{
	// 数据
public:
	uptr<Expr> left;
	uptr<Expr> right;
	Ident      op;

	// 函数
public:
	BinaryExpr(
	    Env *env, Pos2DRange range, uptr<Expr> left, Ident op, uptr<Expr> right)
	    : Expr(env, range)
	    , left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "op":{} , "left":{}, "right":{} }})",
		                   op.dump_json(),
		                   left->dump_json(),
		                   right->dump_json());
	}

private:
	// ITyped
	virtual uptr<Type> solve_type(TypeChecker *tc) const;
};

struct UnaryExpr : public Expr
{
	// 数据
public:
	bool       prefix;
	uptr<Expr> operand;
	Ident      op;

	// 函数
public:
	UnaryExpr(
	    Env *env, Pos2DRange range, bool prefix, uptr<Expr> operand, Ident op)
	    : Expr(env, range)
	    , prefix(prefix)
	    , operand(std::move(operand))
	    , op(std::move(op))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "op":  {} , "right":{} }})",
		                   op.dump_json(),
		                   operand->dump_json());
	}

public:
	// ITyped
	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct CallExpr : public Expr
{
	// 数据
public:
	uptr<Expr>              callee;
	std::vector<uptr<Expr>> args;

public:
	CallExpr(Env                    *env,
	         Pos2DRange              range,
	         uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args)
	    : Expr(env, range)
	    , callee(std::move(callee))
	    , args(std::move(args))
	{}

	std::vector<Type *> arg_types(TypeChecker *tc) const;

	std::string dump_json() const override
	{
		return std::format(R"({{ "callee": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}

public:
	// ITyped
	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct BracketExpr : public CallExpr
{
public:
	BracketExpr(Env                    *env,
	            Pos2DRange              range,
	            uptr<Expr>              callee,
	            std::vector<uptr<Expr>> args)
	    : CallExpr(env, range, std::move(callee), std::move(args))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "indexed": {}, "args": {} }})",
		                   callee->dump_json(),
		                   dump_json_for_vector_of_ptr(args));
	}

	// virtual uptr<Type> solve_type(TypeChecker *);
};

struct MemberAccessExpr : public Expr
{
	// 数据
public:
	uptr<Expr> left;
	Ident      member;

	// 函数
public:
	MemberAccessExpr(Env *env, Pos2DRange range, uptr<Expr> left, Ident member)
	    : Expr(env, range)
	    , left(std::move(left))
	    , member(std::move(member))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "left":{}, "member":{} }})",
		                   left->dump_json(),
		                   member.dump_json());
	}

private:
	// ITyped
	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct LiteralExpr : public Expr
{
public:
	Token token;

public:
	explicit LiteralExpr(Env *env, const Token &token)
	    : Expr(env, token.range())
	    , token(token)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "str":"{}", "int":{}, "fp":{} }})",
		                   token.str_data,
		                   token.int_data,
		                   token.fp_data);
	}

	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct IdentExpr : public Expr
{
public:
	Ident ident;

public:
	explicit IdentExpr(Env *env, Ident ident)
	    : Expr(env, ident.location)
	    , ident(std::move(ident))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident": "{}"  }})", ident.dump_json());
	}

	uptr<Type> solve_type(TypeChecker *tc) const override;
};

struct Decl : public Ast
{
	Ident ident;

	Decl() = default;
	explicit Decl(Env *env, Pos2DRange range, Ident ident)
	    : Ast(env, range)
	    , ident(ident)
	{}
};
class NamedVar;
struct VarDecl : public Decl
{
	uptr<TypeExpr> type;
	uptr<Expr>     init;

	VarDecl() = default;
	VarDecl(Env           *env,
	        Pos2DRange     range,
	        Ident          ident,
	        uptr<TypeExpr> type,
	        uptr<Expr>     init)
	    : Decl(env, range, ident)
	    , type(std::move(type))
	    , init(std::move(init))
	{}

	NamedVar *add_to_env(const NamedEntityProperties &props, Env *env) const;

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident":"{}", "type": {} , "init":{} }})",
		                   ident.dump_json(),
		                   type->dump_json(),
		                   init->dump_json());
	}

	void check_type(TypeChecker *) override;
};

struct ParamDecl : public Decl
{
	uptr<TypeExpr> type;

	ParamDecl() = default;
	ParamDecl(Env *env, Pos2DRange range, Ident ident, uptr<TypeExpr> type)
	    : Decl(env, range, ident)
	    , type(std::move(type))
	{}

	NamedVar *add_to_env(const NamedEntityProperties &props, Env *env) const;

	std::string dump_json() const override
	{
		return std::format(R"({{ "name":"{}", "type": {}  }})",
		                   ident.dump_json(),
		                   type->dump_json());
	}

	void check_type(TypeChecker *) override
	{
		// todo: 实现
	}
};

struct Stmt : public Ast
{
	explicit Stmt(Env *env, Pos2DRange range)
	    : Ast(env, range)
	{}
};

struct ExprStmt : public Stmt
{
	uptr<Expr> expr;

	explicit ExprStmt(Env *env, Pos2DRange range, uptr<Expr> expr)
	    : Stmt(env, range)
	    , expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "expr":{} }})", expr->dump_json());
	}

	void check_type(TypeChecker *tc) override { expr->check_type(tc); }
};

struct CompoundStmtElem
{

	explicit CompoundStmtElem(uptr<Stmt> stmt)
	    : _stmt(std::move(stmt))
	{}

	explicit CompoundStmtElem(uptr<VarDecl> var_decl)
	    : _var_decl(std::move(var_decl))
	{}

	Stmt    *stmt() const { return _stmt.get(); }
	VarDecl *var_decl() const { return _var_decl.get(); }

	std::string dump_json() const
	{
		if (stmt())
			return stmt()->dump_json();
		if (var_decl())
			return var_decl()->dump_json();
		return "null";
	}
	void check_type(TypeChecker *tc)
	{
		if (stmt())
			stmt()->check_type(tc);
		else if (var_decl())
			var_decl()->check_type(tc);
		else
			assert(false); // 没考虑这么多
	}

private:
	uptr<Stmt>    _stmt;
	uptr<VarDecl> _var_decl;
};

struct CompoundStmt : public Stmt
{
public:
	uptr<Env>                           env;
	std::vector<uptr<CompoundStmtElem>> elems;

	CompoundStmt(Env                                *enclosing_env,
	             Pos2DRange                          range,
	             uptr<Env>                           _env,
	             std::vector<uptr<CompoundStmtElem>> elems);

	std::string dump_json() const override
	{
		return dump_json_for_vector_of_ptr(elems);
	}

	void check_type(TypeChecker *tc) override
	{
		for (auto &&elem : elems)
		{
			elem->check_type(tc);
		}
	}
};
class NamedFunc;
struct FuncDecl : public Decl
{
	std::vector<uptr<ParamDecl>> params;
	uptr<TypeExpr>               return_type;
	uptr<CompoundStmt>           body;

	FuncDecl() = default;
	FuncDecl(Env                         *env,
	         Pos2DRange                   range,
	         Ident                        name,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	NamedFunc *add_to_env(const NamedEntityProperties &props, Env *env) const;

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident":{}, "return_type": {} , "body":{} }})",
		                   ident.dump_json(),
		                   return_type->dump_json(),
		                   body->dump_json());
	}

	void check_type(TypeChecker *tc) override
	{
		// todo: params 如果有默认值，可能还得check一下
		// todo: return 必须check
		body->check_type(tc);
	}
};

struct MemberDecl
{
	explicit MemberDecl(uptr<FuncDecl> func_decl);
	explicit MemberDecl(uptr<VarDecl> var_decl);

	FuncDecl *func_decl() const { return _func_decl.get(); }
	VarDecl  *var_decl() const { return _var_decl.get(); }

	std::string dump_json() const
	{
		if (func_decl())
			return func_decl()->dump_json();
		if (var_decl())
			return var_decl()->dump_json();
		return "null";
	}

	void check_type(TypeChecker *tc)
	{
		if (func_decl())
			func_decl()->check_type(tc);
		else if (var_decl())
			var_decl()->check_type(tc);
		else
			assert(false); // 没考虑这么多
	}

private:
	uptr<FuncDecl> _func_decl;
	uptr<VarDecl>  _var_decl;
};

struct StructBody : public Ast
{
	uptr<Env>                     env;
	std::vector<uptr<MemberDecl>> elems;

	StructBody(Env                          *enclosing_env,
	           Pos2DRange                    range,
	           uptr<Env>                     _env,
	           std::vector<uptr<MemberDecl>> elems);

	std::string dump_json() const override
	{
		return dump_json_for_vector_of_ptr(elems);
	}

	void check_type(TypeChecker *tc) override
	{
		for (auto &&elem : elems)
		{
			elem->check_type(tc);
		}
	}
};

struct StructDecl : public Decl
{
	uptr<StructBody> body;

	StructDecl(Env              *env,
	           const Pos2DRange &range,
	           const Ident      &ident,
	           uptr<StructBody>  body);

	NamedType *add_to_env(const NamedEntityProperties &props, Env *env) const;

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident": {},  "body": {} }})",
		                   ident.dump_json(),
		                   body->dump_json());
	}

	void check_type(TypeChecker *) override;
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

	void check_type(TypeChecker *tc) override
	{
		for (auto &&decl : decls)
		{
			decl->check_type(tc);
		}
	}
};

} // namespace protolang