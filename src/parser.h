#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "ast.h"
#include "env.h"
#include "exceptions.h"
#include "token.h"
/*
expression     → assignment
assignment    -> equality "=" assignment
               | equality
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | unary_post
unary_post     → member_access ( "(" args ")" | "[" arg "]" ) *
member_access -> primary ( "." primary )*
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | id
               | "(" expression ")" ;

type_expr      -> "func" "(" type_expr "," type_expr "," ... ")" -> type_expr
                | ident
                | ident < type_expr [, type_expr ...] >

 a.b().c()
 ===
 (((a.b)()).c)()
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
	size_t               index = 0;
	Logger              &logger;
	std::vector<Token>   tokens = {};
	std::unique_ptr<Env> root_env;
	Env                 *curr_env = nullptr;

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
		root_env = std::make_unique<Env>(nullptr);
		curr_env = root_env.get();

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
				    ErrorUnexpectedToken(curr(), "declaration expected"));
				throw ExceptionPanic();
			}
			catch (const ExceptionPanic &)
			{
				sync();
			}
		}
		exit(1);
	}

	uptr<TypeExpr> type_expr()
	{
		eat_ident_or_panic("type");
		auto type_name = prev();
		return std::make_unique<TypeExprIdent>(type_name.str_data);
	}

	uptr<DeclVar> var_decl()
	{
		// var a : int = 2;
		eat_keyword_or_panic(Keyword::KW_VAR);
		auto name_token = eat_ident_or_panic();
		auto name       = name_token.str_data;
		eat_given_type_or_panic(Token::Type::Column, ":");
		auto type = type_expr();
		eat_op_or_panic("=");
		auto init = expression();
		eat_given_type_or_panic(Token::Type::SemiColumn, ";");

		// 产生符号表记录
		NamedObjectProperties p;
		p.ident_pos     = name_token.range();
		p.available_pos = curr().range();
		return add_name_to_curr_env(
		    std::make_unique<DeclVar>(name, std::move(type), std::move(init)),
		    p);
	}

	uptr<DeclFunc> func_decl()
	{
		// func foo(arg1: int, arg2: int) -> int { ... }
		eat_keyword_or_panic(KW_FUNC);
		Token       func_name_tok = eat_ident_or_panic();
		std::string func_name     = func_name_tok.str_data;

		eat_given_type_or_panic(Token::Type::LeftParen, "(");

		std::vector<uptr<DeclParam>> params;
		// arg1: int, arg2: int,)
		// arg1: int, arg2: int)
		while (!is_curr_of_type(Token::Type::RightParen))
		{
			auto param_name = eat_ident_or_panic().str_data;
			eat_given_type_or_panic(Token::Type::Column, ":");
			auto type = type_expr();
			params.push_back(
			    std::make_unique<DeclParam>(param_name, std::move(type)));
			// 下一个必须是`,`或者`)`
			if (!is_curr_of_type(Token::Type::RightParen))
			{
				eat_given_type_or_panic(Token::Type::Comma, ",");
			}
		}
		eat_given_type_or_panic(Token::Type::RightParen, ")");
		eat_given_type_or_panic(Token::Type::Arrow, "->");
		auto return_type = type_expr();
		auto body        = compound_statement();

		auto decl = std::make_unique<DeclFunc>(std::move(func_name),
		                                       std::move(params),
		                                       std::move(return_type),
		                                       std::move(body));
		NamedObjectProperties props;
		props.ident_pos     = func_name_tok.range();
		props.available_pos = curr().range();
		return add_name_to_curr_env(std::move(decl), props);
	}

	// 加入符号表
	template <typename DeclType>
	uptr<DeclType> add_name_to_curr_env(uptr<DeclType>               decl,
	                                    const NamedObjectProperties &props)
	{

		auto        named_obj = decl->declare(props);
		std::string name      = named_obj->name;
		if (curr_env->add(name, std::move(named_obj)))
		{
			//			std::ofstream(R"(D:\Projects\protolang\test\.dump.json)")
			//			    << curr_env->dump_json() << "\n";
			return decl;
		}
		else
		{
			logger.log(ErrorSymbolRedefinition(
			    name, curr_env->get(name)->props.ident_pos, props.ident_pos));
			throw ExceptionPanic();
		}
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
		auto     env = std::make_unique<Env>(curr_env);
		EnvGuard guard(curr_env, env.get());
		auto     stmt = std::make_unique<StmtCompound>(std::move(env));
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

	uptr<Expr> expression() { return assignment(); }

	uptr<Expr> assignment()
	{
		uptr<Expr> left = equality();
		if (eat_if_is_given_op({"="}))
		{
			uptr<Expr> right = assignment();
			return uptr<Expr>(new ExprBinary(Expr::ValueCat::Lvalue,
			                                 std::move(left),
			                                 "=",
			                                 std::move(right)));
		}
		return left;
	}

	uptr<Expr> equality()
	{
		uptr<Expr> expr = comparison();
		while (eat_if_is_given_op({"==", "!="}))
		{
			Token      op    = prev();
			uptr<Expr> right = comparison();
			expr             = uptr<Expr>(new ExprBinary(Expr::ValueCat::Rvalue,
                                             std::move(expr),
                                             op.str_data,
                                             std::move(right)));
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
			expr             = uptr<Expr>(new ExprBinary(Expr::ValueCat::Rvalue,
                                             std::move(expr),
                                             op.str_data,
                                             std::move(right)));
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
			expr             = uptr<Expr>(new ExprBinary(Expr::ValueCat::Rvalue,
                                             std::move(expr),
                                             op.str_data,
                                             std::move(right)));
		}
		return expr;
	}

	uptr<Expr> factor()
	{
		uptr<Expr> expr = unary_pre();
		while (eat_if_is_given_op({"*", "/", "%"}))
		{
			Token      op    = prev();
			uptr<Expr> right = unary_pre();
			expr             = uptr<Expr>(new ExprBinary(Expr::ValueCat::Rvalue,
                                             std::move(expr),
                                             op.str_data,
                                             std::move(right)));
		}
		return expr;
	}

	uptr<Expr> unary_pre()
	{
		if (eat_if_is_given_op({"!", "-"}))
		{
			auto       op    = prev().str_data;
			uptr<Expr> right = unary_pre();
			return uptr<Expr>(new ExprUnary(
			    Expr::ValueCat::Rvalue, true, std::move(right), std::move(op)));
		}
		else
		{
			return unary_post();
		}
	}

	uptr<Expr> unary_post()
	{
		uptr<Expr> operand = member_access();
		// matrix[1][2](arg1, arg2)
		while (eat_if_is_given_type(
		    {Token::Type::LeftParen, Token::Type::LeftBracket}))
		{
			bool        isCall = (prev().type == Token::Type::LeftParen);
			Token::Type rightDelim =
			    isCall ? Token::Type::RightParen : Token::Type::RightBracket;

			std::vector<uptr<Expr>> args;
			// arg1, arg2,)
			// arg1, arg2 )
			while (!is_curr_of_type(rightDelim))
			{
				auto expr = expression();
				args.push_back(std::move(expr));
				if (!is_curr_of_type(rightDelim))
				{
					// 不是右括号就必须是逗号
					// 是逗号，就吃掉
					eat_given_type_or_panic(Token::Type::Comma, ",");
				}
				// 是右括号就结束
			}
			// 吃掉右括号
			eat_given_type_or_panic(rightDelim, isCall ? ")" : "]");
			if (isCall)
				operand = uptr<Expr>(
				    new ExprCall(std::move(operand), std::move(args)));
			else
				operand = uptr<Expr>(
				    new ExprIndex(std::move(operand), std::move(args)));
		}
		return operand;
	}

	uptr<Expr> member_access()
	{
		uptr<Expr> expr = primary();
		while (eat_if_is_given_op({"."}))
		{
			uptr<Expr> rhs = primary();
			expr           = uptr<Expr>(new ExprBinary(
                Expr::ValueCat::Lvalue, std::move(expr), ".", std::move(rhs)));
		}
		return expr;
	}

	uptr<Expr> primary()
	{
		if (eat_if_is_given_type({Token::Type::Id}))
		{
			return uptr<Expr>(new ExprIdent(prev().str_data));
		}
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

	const Token &eat_ident_or_panic(const std::string &expected = "identifier")
	{
		return eat_given_type_or_panic(Token::Type::Id, expected, false);
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