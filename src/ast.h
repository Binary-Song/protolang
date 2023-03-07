#pragma once
#include <format>
#include <utility>
#include "token.h"
namespace protolang
{

struct Ast
{
public:
	virtual std::string dump_json() const = 0;
};

struct Expr : public Ast
{
public:
	virtual ~Expr() = default;
};

struct ExprBinary : public Expr
{
public:
	uptr<Expr> left;
	uptr<Expr> right;
	Token      op;

public:
	ExprBinary(uptr<Expr> left, Token op, uptr<Expr> right)
	    : left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "op":"{}", "left":{}, "right":{} }})",
		                   op.str_data,
		                   left->dump_json(),
		                   right->dump_json());
	}
};

struct ExprUnary : public Expr
{
public:
	uptr<Expr> right;
	Token      op;

public:
	ExprUnary(uptr<Expr> right, Token op)
	    : right(std::move(right))
	    , op(std::move(op))
	{}

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "op":"{}", "right":{} }})", op.str_data, right->dump_json());
	}
};

/// 括号
struct ExprGroup : public Expr
{
public:
	uptr<Expr> expr;

public:
	explicit ExprGroup(uptr<Expr> expr)
	    : expr(std::move(expr))
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
	    : token(token)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "str":"{}", "int":{}, "fp":{} }})",
		                   token.str_data,
		                   token.int_data,
		                   token.fp_data);
	}
};

struct Stmt : public Ast
{};

struct Decl : public Ast
{};

struct StmtExprStmt : public Ast
{
	uptr<Expr> expr;

	explicit StmtExprStmt(uptr<Expr> expr)
	    : expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "expr":{} }})", expr->dump_json());
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
		std::string json;
		for (auto &&item : decls)
		{
			json += item->dump_json();
			json += ",";
		}
		if (json.ends_with(','))
		{
			json.pop_back();
		}
		return std::format(R"({{ "decls":[ {} ] }})", json);
	}
};

} // namespace protolang