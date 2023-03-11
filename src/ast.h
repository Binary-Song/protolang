#pragma once
#include <cassert>
#include <format>
#include <functional>
#include <utility>
#include <vector>
#include "entity_system.h"
#include "ident.h"
#include "token.h"
#include "util.h"
namespace protolang
{
struct IType;
class Env;
class TypeChecker;

struct Ast : virtual IJsonDumper
{
	// 虚函数
public:
	virtual ~Ast()                               = default;
	[[nodiscard]] virtual SrcRange range() const = 0;
	[[nodiscard]] virtual Env     *env() const   = 0;
};

// 类型表达式，这是类型的引用，并不是真正的类型声明，抽象类
struct TypeExpr : Ast, ITyped
{};

// 类型标识符
struct IdentTypeExpr : TypeExpr
{
	// 数据
private:
	Ident m_ident;
	Env  *m_env;
	// 函数
public:
	explicit IdentTypeExpr(Env *env, Ident ident);
	[[nodiscard]] const Ident &ident() const { return m_ident; }
	// 实现基类成员
	[[nodiscard]] std::string  dump_json() const override
	{
		return std::format(
		    R"({{"obj":"IdentTypeExpr","ident":{}}})",
		    m_ident.dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range;
	}
	[[nodiscard]] Env   *env() const override { return m_env; }
	[[nodiscard]] IType *get_type() override;
};

// 表达式，抽象类
struct Expr : public Ast, public ITyped
{
	// 类
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	};

	[[nodiscard]] virtual ValueCat get_value_cat() const
	{
		return ValueCat::Pending;
	}
};

// 二元运算表达式
struct BinaryExpr : public Expr
{
	// 数据
public:
	uptr<Expr> left;
	uptr<Expr> right;
	Ident      op;

	// 函数
public:
	BinaryExpr(uptr<Expr> left, Ident op, uptr<Expr> right)
	    : left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}
	[[nodiscard]] SrcRange range() const override
	{
		return left->range() + right->range();
	}
	[[nodiscard]] Env *env() const override
	{
		assert(left->env() == right->env());
		return left->env();
	}
	[[nodiscard]] IType      *get_type() override;
	[[nodiscard]] std::string dump_json() const override
	{
		return std::format(
		    R"({{"obj":"BinaryExpr","op":{},"lhs":{},"rhs":{}}})",
		    op.dump_json(),
		    left->dump_json(),
		    right->dump_json());
	}
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
	UnaryExpr(bool prefix, uptr<Expr> operand, Ident op)
	    : prefix(prefix)
	    , operand(std::move(operand))
	    , op(std::move(op))
	{}

	[[nodiscard]] std::string dump_json() const override
	{
		return std::format(
		    R"({{"obj":"UnaryExpr","op":{},"oprd":{}}})",
		    op.dump_json(),
		    operand->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return op.range + operand->range();
	}
	[[nodiscard]] Env *env() const override
	{
		return operand->env();
	}
	[[nodiscard]] IType *get_type() override;
};

struct CallExpr : public Expr
{
	// 数据
public:
	uptr<Expr>              callee;
	std::vector<uptr<Expr>> args;
	const SrcRange         &src_rng;

public:
	CallExpr(const SrcRange         &src_rng,
	         uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args)
	    : callee(std::move(callee))
	    , args(std::move(args))
	    , src_rng(src_rng)
	{}

	std::vector<IType *> arg_types() const;

	std::string dump_json() const override
	{
		return std::format(
		    R"({{"obj":"CallExpr","callee":{},"args":{}}})",
		    callee->dump_json(),
		    dump_json_for_vector_of_ptr(args));
	}
	SrcRange range() const override { return src_rng; }
	Env     *env() const override { return callee->env(); }
	IType   *get_type() override;
};

struct BracketExpr : public CallExpr
{
public:
	BracketExpr(Env                    *env,
	            SrcRange                range,
	            uptr<Expr>              callee,
	            std::vector<uptr<Expr>> args)
	    : CallExpr(
	          env, range, std::move(callee), std::move(args))
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
	MemberAccessExpr(Env       *env,
	                 SrcRange   range,
	                 uptr<Expr> left,
	                 Ident      member)
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
	uptr<IType> solve_type(TypeChecker *tc) const override;
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
		return std::format(
		    R"({{ "str":"{}", "int":{}, "fp":{} }})",
		    token.str_data,
		    token.int_data,
		    token.fp_data);
	}

	uptr<IType> solve_type(TypeChecker *tc) const override;
};

struct IdentExpr : public Expr
{
public:
	Ident ident;

public:
	explicit IdentExpr(Env *env, Ident ident)
	    : Expr(env, ident.range)
	    , ident(std::move(ident))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "ident": "{}"  }})",
		                   ident.dump_json());
	}

	uptr<IType> solve_type(TypeChecker *tc) const override;
};

struct Decl : public Ast
{
	Ident ident;

	Decl() = default;
	explicit Decl(Env *env, SrcRange range, Ident ident)
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
	        SrcRange       range,
	        Ident          ident,
	        uptr<TypeExpr> type,
	        uptr<Expr>     init)
	    : Decl(env, range, ident)
	    , type(std::move(type))
	    , init(std::move(init))
	{}

	NamedVar *add_to_env(const NamedEntityProperties &props,
	                     Env                         *env) const;

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "ident":"{}", "type": {} , "init":{} }})",
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
	ParamDecl(Env           *env,
	          SrcRange       range,
	          Ident          ident,
	          uptr<TypeExpr> type)
	    : Decl(env, range, ident)
	    , type(std::move(type))
	{}

	NamedVar *add_to_env(const NamedEntityProperties &props,
	                     Env                         *env) const;

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
	explicit Stmt(Env *env, SrcRange range)
	    : Ast(env, range)
	{}
};

struct ExprStmt : public Stmt
{
	uptr<Expr> expr;

	explicit ExprStmt(Env *env, SrcRange range, uptr<Expr> expr)
	    : Stmt(env, range)
	    , expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "expr":{} }})",
		                   expr->dump_json());
	}

	void check_type(TypeChecker *tc) override
	{
		expr->check_type(tc);
	}
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

struct CompoundStmt : public Stmt, public IFuncBody
{
public:
	uptr<Env>                           env;
	std::vector<uptr<CompoundStmtElem>> elems;

	CompoundStmt(Env      *enclosing_env,
	             SrcRange  range,
	             uptr<Env> _env,
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
	         SrcRange                     range,
	         Ident                        name,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	NamedFunc *add_to_env(const NamedEntityProperties &props,
	                      Env *env) const;

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "ident":{}, "return_type": {} , "body":{} }})",
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
	           SrcRange                      range,
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

	StructDecl(Env             *env,
	           const SrcRange  &range,
	           const Ident     &ident,
	           uptr<StructBody> body);

	NamedType *add_to_env(const NamedEntityProperties &props,
	                      Env *env) const;

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
	Env                    *root_env;

	explicit Program(std::vector<uptr<Decl>> decls,
	                 Env                    *root_env);

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