#pragma once
#include <functional>
#include <vector>
#include "ast.h"
#include "exceptions.h"
#include "token.h"
/*
expression     → equality ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | primary ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" ;
*/
namespace protolang
{
class Parser
{
public:
	explicit Parser(std::vector<Token> tokens, Logger &logger)
	    : tokens(std::move(tokens))
	    , logger(logger)
	{}

	uptr<Expr> parse()
	{
		try
		{
			return expression();
		}
		catch (const ExceptionPanic &)
		{
			// todo: 改为继续parse。
			return nullptr;
		}
	}

private:
	size_t             index = 0;
	Logger            &logger;
	std::vector<Token> tokens = {};

private:
	const Token &curr() const { return tokens[index]; }
	const Token &prev() const { return tokens[index - 1]; }

	void sync()
	{
		while (curr().type != Token::Type::Eof)
		{
			index++;

			if (prev().type == Token::Type::SemiColumn)
			{
				return; // 同步成功
			}
		}
	}

	uptr<Expr> expression() { return equality(); }

	uptr<Expr> equality()
	{
		uptr<Expr> expr = comparison();
		while (match_op({"==", "!="}))
		{
			Token      op    = prev();
			uptr<Expr> right = comparison();
			expr             = uptr<Expr>(
                new ExprBinary(std::move(expr), op, std::move(right)));
		}
		return expr;
	}

	uptr<Expr> comparison()
	{
		uptr<Expr> expr = term();
		while (match_op({">", ">=", "<", "<="}))
		{
			Token      op    = prev();
			uptr<Expr> right = term();
			expr             = uptr<Expr>(
                new ExprBinary(std::move(expr), op, std::move(right)));
		}
		return expr;
	}

	uptr<Expr> term()
	{
		uptr<Expr> expr = factor();
		while (match_op({"+", "-"}))
		{
			Token      op    = prev();
			uptr<Expr> right = factor();
			expr             = uptr<Expr>(
                new ExprBinary(std::move(expr), op, std::move(right)));
		}
		return expr;
	}

	uptr<Expr> factor()
	{
		uptr<Expr> expr = unary();
		while (match_op({"*", "/", "%"}))
		{
			Token      op    = prev();
			uptr<Expr> right = unary();
			expr             = uptr<Expr>(
                new ExprBinary(std::move(expr), op, std::move(right)));
		}
		return expr;
	}

	uptr<Expr> unary()
	{
		if (match_op({"!", "-"}))
		{
			Token      op    = prev();
			uptr<Expr> right = unary();
			return uptr<Expr>(new ExprUnary(std::move(right), std::move(op)));
		}
		else
		{
			return primary();
		}
	}

	uptr<Expr> primary()
	{
		if (match_type({Token::Type::Str, Token::Type::Int, Token::Type::Fp}))
		{
			return uptr<Expr>(new ExprLiteral(prev()));
		}
		if (match_type({Token::Type::LeftParen}))
		{
			Token      left_paren = prev();
			uptr<Expr> expr       = expression();
			if (match_type({Token::Type::RightParen}))
			{
				return uptr<Expr>(new ExprGroup(std::move(expr)));
			}
			else
			{
				// error : left parentheses mismatch
				logger.log(ErrorParenMismatch(true, left_paren));
				throw ExceptionPanic();
			}
		}
		if (curr().type == Token::Type::RightParen)
		{
			// error : left parentheses mismatch
			logger.log(ErrorParenMismatch(false, curr()));
		}
		else
		{
			logger.log(ErrorUnexpectedToken(curr()));
		}
		throw ExceptionPanic();
	}

	/// 看看curr是不是指定操作符之一。见match
	bool match_op(std::initializer_list<const char *> ops)
	{
		return match(
		    [ops](const Token &token)
		    {
			    if (token.type != Token::Type::Op)
				    return false;
			    for (auto op : ops)
			    {
				    if (token.str_data == op)
					    return true;
			    }
			    return false;
		    });
	}

	bool match_type(std::initializer_list<Token::Type> types)
	{
		return match(
		    [types](const Token &token)
		    {
			    for (auto ty : types)
			    {
				    if (token.type == ty)
					    return true;
			    }
			    return false;
		    });
	}

	/// 看看curr满不满足标准，如果criteria返回true，curr前进一步，返回true；否则，curr不变，返回false。
	bool match(std::function<bool(const Token &)> criteria)
	{
		if (criteria(curr()) == true)
		{
			index++;
			return true;
		}
		return false;
	}
};
} // namespace protolang