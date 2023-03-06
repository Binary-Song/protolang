#pragma once
#include <format>
#include <utility>
#include "token.h"
namespace protolang
{
struct Expr
{
public:
	virtual ~Expr()                       = default;
	virtual std::string dump_json() const = 0;
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

} // namespace protolang