#pragma once
#include <concepts>
#include <functional>
#include <memory>
#include <vector>
#include "ast.h"
#include "env.h"
#include "exceptions.h"
#include "ident.h"
#include "logger.h"
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
// 调用、下标 member_access -> primary ( "." primary )*
 primary
→ NUMBER | STRING | "true" | "false" | "nil" | id | "("
expression ")" ;

type_expr      -> "func" "(" type_expr "," type_expr "," ... ")"
-> type_expr | ident | ident < type_expr [, type_expr ...] >

 a.b().c()
 ===
 (((a.b)()).c)()
 */
namespace protolang
{
class Parser
{
	// 数据
private:
	size_t             index = 0;
	Logger            &logger;
	std::vector<Token> tokens = {};
	Env               *root_env;
	Env               *curr_env = nullptr;

public:
	explicit Parser(Logger            &logger,
	                std::vector<Token> tokens,
	                Env               *root_env)
	    : tokens(std::move(tokens))
	    , logger(logger)
	    , root_env(root_env)
	{}

	uptr<ast::Program> parse() { return program(); }

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

	uptr<ast::Program>      program();
	uptr<ast::Decl>         declaration();
	uptr<ast::TypeExpr>     type_expr();
	uptr<ast::VarDecl>      var_decl();
	uptr<ast::FuncDecl>     func_decl();
	uptr<ast::StructDecl>   struct_decl();
	uptr<ast::Stmt>         statement();
	uptr<ast::ReturnStmt>   return_statement();
	uptr<ast::ExprStmt>     expression_statement();
	uptr<ast::CompoundStmt> compound_statement();
	uptr<ast::StructBody>   struct_body();
	uptr<ast::Expr>         expression();
	uptr<ast::Expr>         assignment();
	uptr<ast::Expr>         equality();
	uptr<ast::Expr>         comparison();
	uptr<ast::Expr>         term();
	uptr<ast::Expr>         factor();
	uptr<ast::Expr>         unary_pre();
	uptr<ast::Expr>         unary_post();
	uptr<ast::Expr>         member_access();
	uptr<ast::Expr>         primary();
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

	// 解析一个块，T为具体是什么块，elem_handler用来解析块中的一条语句
	// (不同的块对块中的语句可能有不同的要求)

	void parse_block(
	    ast::IBlock                       *block,
	    std::function<void(ast::IBlock *)> elem_handler);

	bool is_curr_eof() const
	{
		return curr().type == Token::Type::Eof;
	}
	bool is_curr_decl_keyword() const
	{
		if (curr().type != Token::Type::Keyword)
			return false;
		return (curr().int_data == KW_VAR ||
		        curr().int_data == KW_STRUCT ||
		        curr().int_data == KW_CLASS ||
		        curr().int_data == KW_FUNC);
	}
	bool is_curr_of_type(Token::Type type) const
	{
		return curr().type == type;
	}
	bool is_curr_keyword() const
	{
		return curr().type == Token::Type::Keyword;
	}
	bool is_curr_keyword(Keyword kw) const
	{
		return is_curr_keyword() && curr().int_data == kw;
	}
	bool is_curr_ident() const
	{
		return curr().type == Token::Type::Id;
	}

	const Token &eat_or_panic(
	    std::function<bool(const Token &)> criteria,
	    const std::string                 &expected)
	{
		if (!criteria(curr()))
		{
			ErrorUnexpectedToken e;
			e.expected = expected;
			e.range    = curr().range();
			throw std::move(e);
		}
		else
		{
			index++;
			return prev();
		}
	}

	const Token &eat_given_type_or_panic(
	    Token::Type        type,
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
			    return token.type == Token::Type::Op &&
			           token.str_data == op;
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

	const Token &eat_ident_or_panic(
	    const std::string &expected = "identifier")
	{
		return eat_given_type_or_panic(
		    Token::Type::Id, expected, false);
	}

	/// 看看curr是不是指定操作符之一。
	bool eat_if_is_given_op(
	    std::initializer_list<const char *> ops)
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

	bool eat_if_is_given_type(
	    std::initializer_list<Token::Type> types)
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
	bool eat_if(
	    const std::function<bool(const Token &)> &criteria)
	{
		if (criteria(curr()))
		{
			index++;
			return true;
		}
		return false;
	}
};

} // namespace protolang