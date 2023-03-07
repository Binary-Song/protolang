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

	uptr<Program> parse() { return program(); }

private:
	size_t             index = 0;
	Logger            &logger;
	std::vector<Token> tokens = {};

private:
	const Token &curr() const { return tokens[index]; }
	const Token &prev() const { return tokens[index - 1]; }

	void sync()
	{
		while (!is_curr_eof())
		{
			index++;

			if (prev().type == Token::Type::RightBrace)
			{
				return; // 同步成功
			}
		}
	}

	uptr<Program> program()
	{
		std::vector<uptr<Decl>> vec;
		while (!is_curr_eof())
		{
			vec.push_back(declaration());
		}
		return std::make_unique<Program>(std::move(vec));
	}

	uptr<Decl> declaration()
	{
		while (!is_curr_eof())
		{
			try
			{
				if (is_curr_keyword(Keyword::KW_VAR))
				{
					return var_decl();
				}
				else if (is_curr_keyword(Keyword::KW_FUNC))
				{
					return func_decl();
				}
				logger.log(
				    ErrorUnexpectedToken(curr(), "`var` or `func` expected"));
				throw ExceptionPanic();
			}
			catch (const ExceptionPanic &)
			{
				sync();
			}
		}
		exit(1);
	}

	uptr<DeclVar> var_decl()
	{
		// var a : int = 2;
		eat_keyword_or_panic(Keyword::KW_VAR);
		auto name = eat_ident_or_panic().str_data;
		eat_given_type_or_panic(Token::Type::Column, ":");
		auto type = eat_ident_or_panic().str_data;
		eat_op_or_panic("=");
		auto init = expression();
		eat_given_type_or_panic(Token::Type::SemiColumn, ";");
		return uptr<DeclVar>(new DeclVar(name, type, std::move(init)));
	}

	uptr<DeclFunc> func_decl()
	{
		// func foo(arg1: int, arg2: int) -> int { ... }
		eat_keyword_or_panic(KW_FUNC);
		auto name = eat_ident_or_panic().str_data;
		eat_given_type_or_panic(Token::Type::LeftParen, "(");

		std::vector<uptr<DeclParam>> params;
		// arg1: int, arg2: int,)
		// arg1: int, arg2: int)
		while (!is_curr_of_type(Token::Type::RightParen))
		{
			auto param_name = eat_ident_or_panic().str_data;
			eat_given_type_or_panic(Token::Type::Column, ":");
			auto type = eat_ident_or_panic().str_data;
			params.push_back(std::make_unique<DeclParam>(param_name, type));
			// 下一个必须是`,`或者`)`
			if (!is_curr_of_type(Token::Type::RightParen))
			{
				eat_given_type_or_panic(Token::Type::Comma, ",");
			}
		}
		eat_given_type_or_panic(Token::Type::RightParen, ")");
		eat_given_type_or_panic(Token::Type::Arrow, "->");
		auto return_type = eat_ident_or_panic().str_data;
		auto body        = compound_statement();
		return std::make_unique<DeclFunc>(std::move(name),
		                                  std::move(params),
		                                  std::move(return_type),
		                                  std::move(body));
	}

	uptr<Stmt> statement()
	{
		if (is_curr_of_type(Token::Type::LeftBrace))
		{
			return compound_statement();
		}
		else
		{
			return expression_statement();
		}
	}

	uptr<StmtExpr> expression_statement()
	{
		auto expr = expression();
		eat_given_type_or_panic(Token::Type::SemiColumn, ";");
		return std::make_unique<StmtExpr>(std::move(expr));
	}

	uptr<StmtCompound> compound_statement()
	{
		auto stmt = std::make_unique<StmtCompound>();
		eat_given_type_or_panic(Token::Type::LeftBrace, "{");
		while (!is_curr_of_type(Token::Type::RightBrace))
		{
			if (is_curr_keyword(Keyword::KW_VAR))
			{
				auto elem = std::make_unique<CompoundStmtElem>(var_decl());
				stmt->elems.push_back(std::move(elem));
			}
			else
			{
				auto elem = std::make_unique<CompoundStmtElem>(statement());
				stmt->elems.push_back(std::move(elem));
			}
		}
		eat_given_type_or_panic(Token::Type::RightBrace, "}");
		return stmt;
	}

	uptr<Expr> expression() { return equality(); }

	uptr<Expr> equality()
	{
		uptr<Expr> expr = comparison();
		while (eat_if_is_given_op({"==", "!="}))
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
		while (eat_if_is_given_op({">", ">=", "<", "<="}))
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
		while (eat_if_is_given_op({"+", "-"}))
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
		while (eat_if_is_given_op({"*", "/", "%"}))
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
		if (eat_if_is_given_op({"!", "-"}))
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
		if (eat_if_is_given_type(
		        {Token::Type::Str, Token::Type::Int, Token::Type::Fp}))
		{
			return uptr<Expr>(new ExprLiteral(prev()));
		}
		if (eat_if_is_given_type({Token::Type::LeftParen}))
		{
			Token      left_paren = prev();
			uptr<Expr> expr       = expression();
			if (eat_if_is_given_type({Token::Type::RightParen}))
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
			logger.log(ErrorExpressionExpected(curr()));
		}
		throw ExceptionPanic();
	}

	/*
	 *
	 *
	 *
	 *
	 *                              辅助函数
	 *
	 *
	 *
	 * */

	bool is_curr_eof() const { return curr().type == Token::Type::Eof; }
	bool is_curr_decl_keyword() const
	{
		if (curr().type != Token::Type::Keyword)
			return false;
		return (curr().int_data == KW_VAR || curr().int_data == KW_STRUCT ||
		        curr().int_data == KW_CLASS || curr().int_data == KW_FUNC);
	}
	bool is_curr_of_type(Token::Type type) const { return curr().type == type; }
	bool is_curr_keyword() const { return curr().type == Token::Type::Keyword; }
	bool is_curr_keyword(Keyword kw) const
	{
		return is_curr_keyword() && curr().int_data == kw;
	}
	bool is_curr_ident() const { return curr().type == Token::Type::Id; }

	const Token &eat_or_panic(std::function<bool(const Token &)> criteria,
	                          const std::string                 &expected)
	{
		if (!criteria(curr()))
		{
			logger.log(
			    ErrorUnexpectedToken(curr(), expected + " expected here."));
			throw ExceptionPanic();
		}
		else
		{
			index++;
			return prev();
		}
	}

	const Token &eat_given_type_or_panic(Token::Type        type,
	                                     const std::string &expected,
	                                     bool               add_quotes = true)
	{
		if (add_quotes)
			return eat_or_panic(
			    [type](const Token &token)
			    {
				    return token.type == type;
			    },
			    "`" + expected + "`");
		else
			return eat_or_panic(
			    [type](const Token &token)
			    {
				    return token.type == type;
			    },
			    expected);
	}

	const Token &eat_op_or_panic(const std::string &op)
	{
		return eat_or_panic(
		    [op](const Token &token)
		    {
			    return token.type == Token::Type::Op && token.str_data == op;
		    },
		    "`" + op + "`");
	}

	const Token &eat_keyword_or_panic(Keyword kw)
	{
		return eat_or_panic(
		    [kw](const Token &token)
		    {
			    return token.type == Token::Type::Keyword &&
			           token.int_data == kw;
		    },
		    "`" + kw_map_rev(kw) + "`");
	}

	const Token &eat_ident_or_panic()
	{
		return eat_given_type_or_panic(Token::Type::Id, "identifier", false);
	}

	/// 看看curr是不是指定操作符之一。
	bool eat_if_is_given_op(std::initializer_list<const char *> ops)
	{
		return eat_if(
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

	bool eat_if_is_given_type(std::initializer_list<Token::Type> types)
	{
		return eat_if(
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
	bool eat_if(std::function<bool(const Token &)> criteria)
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