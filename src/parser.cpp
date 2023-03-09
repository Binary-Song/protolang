#include "parser.h"
#include "env.h"
#include "log.h"
#include "type.h"

namespace protolang
{
Program::Program(std::vector<uptr<Decl>> decls, uptr<Env> root_env)
    : Ast(root_env.get())
    , decls(std::move(decls))
    , root_env(std::move(root_env))
{}

uptr<VarDecl> Parser::var_decl()
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
	NamedEntityProperties p;
	p.ident_pos     = name_token.range();
	p.available_pos = curr().range();
	auto decl       = std::make_unique<VarDecl>(curr_env,
                                          Ident(name, name_token.range()),
                                          std::move(type),
                                          std::move(init));
	add_name_to_curr_env(decl.get(), p);
	return decl;
}

uptr<ExprStmt> Parser::expression_statement()
{
	auto expr = expression();
	eat_given_type_or_panic(Token::Type::SemiColumn, ";");
	return std::make_unique<ExprStmt>(curr_env, std::move(expr));
}
uptr<CompoundStmt> Parser::compound_statement()
{
	auto     env  = std::make_unique<Env>(curr_env, logger);
	auto     stmt = std::make_unique<CompoundStmt>(curr_env, std::move(env));
	EnvGuard guard(curr_env, env.get());
	eat_given_type_or_panic(Token::Type::LeftBrace, "{");
	while (!is_curr_of_type(Token::Type::RightBrace))
	{
		if (is_curr_keyword(Keyword::KW_VAR))
		{
			auto elem =
			    std::make_unique<CompoundStmtElem>(curr_env, var_decl());
			stmt->elems.push_back(std::move(elem));
		}
		else
		{
			auto elem =
			    std::make_unique<CompoundStmtElem>(curr_env, statement());
			stmt->elems.push_back(std::move(elem));
		}
	}
	eat_given_type_or_panic(Token::Type::RightBrace, "}");
	return stmt;
}
uptr<Expr> Parser::expression()
{
	return assignment();
}
uptr<Expr> Parser::assignment()
{
	uptr<Expr> left = equality();
	if (eat_if_is_given_op({"="}))
	{
		Token      op    = prev();
		uptr<Expr> right = assignment();
		return uptr<Expr>(new BinaryExpr(curr_env,
		                                 std::move(left),
		                                 Ident(op.str_data, op.range()),
		                                 std::move(right)));
	}
	return left;
}
uptr<Expr> Parser::equality()
{
	uptr<Expr> expr = comparison();
	while (eat_if_is_given_op({"==", "!="}))
	{
		Token      op    = prev();
		uptr<Expr> right = comparison();
		expr             = uptr<Expr>(new BinaryExpr(curr_env,
                                         std::move(expr),
                                         Ident(op.str_data, op.range()),
                                         std::move(right)));
	}
	return expr;
}
uptr<Expr> Parser::comparison()
{
	uptr<Expr> expr = term();
	while (eat_if_is_given_op({">", ">=", "<", "<="}))
	{
		Token      op    = prev();
		uptr<Expr> right = term();
		expr             = uptr<Expr>(new BinaryExpr(curr_env,
                                         std::move(expr),
                                         Ident(op.str_data, op.range()),
                                         std::move(right)));
	}
	return expr;
}
uptr<Expr> Parser::term()
{
	uptr<Expr> expr = factor();
	while (eat_if_is_given_op({"+", "-"}))
	{
		Token      op    = prev();
		uptr<Expr> right = factor();
		expr             = uptr<Expr>(new BinaryExpr(curr_env,
                                         std::move(expr),
                                         Ident(op.str_data, op.range()),
                                         std::move(right)));
	}
	return expr;
}
uptr<Expr> Parser::factor()
{
	uptr<Expr> expr = unary_pre();
	while (eat_if_is_given_op({"*", "/", "%"}))
	{
		Token      op    = prev();
		uptr<Expr> right = unary_pre();
		expr             = uptr<Expr>(new BinaryExpr(curr_env,
                                         std::move(expr),
                                         Ident(op.str_data, op.range()),
                                         std::move(right)));
	}
	return expr;
}
uptr<Expr> Parser::unary_pre()
{
	if (eat_if_is_given_op({"!", "-"}))
	{
		auto       op    = prev();
		uptr<Expr> right = unary_pre();
		return uptr<Expr>(new UnaryExpr(curr_env, //
		                                true,
		                                std::move(right),
		                                Ident(op.str_data, op.range())));
	}
	else
	{
		return unary_post();
	}
}
uptr<Expr> Parser::unary_post()
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
			    new CallExpr(curr_env, std::move(operand), std::move(args)));
		else
			operand = uptr<Expr>(
			    new BracketExpr(curr_env, std::move(operand), std::move(args)));
	}
	return operand;
}
uptr<Expr> Parser::member_access()
{
	uptr<Expr> expr = primary();
	while (eat_if_is_given_op({"."}))
	{
		auto       op  = prev();
		uptr<Expr> rhs = primary();
		expr           = uptr<Expr>(new BinaryExpr(curr_env,
                                         std::move(expr),
		                                           {op.str_data, op.range()},
                                         std::move(rhs)));
	}
	return expr;
}
uptr<Expr> Parser::primary()
{
	if (eat_if_is_given_type({Token::Type::Id}))
	{
		return uptr<Expr>(
		    new IdentExpr(curr_env, Ident(prev().str_data, prev().range())));
	}
	if (eat_if_is_given_type(
	        {Token::Type::Str, Token::Type::Int, Token::Type::Fp}))
	{
		return uptr<Expr>(new LiteralExpr(curr_env, prev()));
	}
	if (eat_if_is_given_type({Token::Type::LeftParen}))
	{
		Token      left_paren = prev();
		uptr<Expr> expr       = expression();
		if (eat_if_is_given_type({Token::Type::RightParen}))
		{
			return uptr<Expr>(new GroupedExpr(curr_env, std::move(expr)));
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
uptr<Decl> Parser::declaration()
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
			logger.log(ErrorUnexpectedToken(curr(), "declaration expected"));
			throw ExceptionPanic();
		}
		catch (const ExceptionPanic &)
		{
			sync();
		}
	}
	exit(1);
}
uptr<FuncDecl> Parser::func_decl()
{
	// func foo(arg1: int, arg2: int) -> int { ... }
	eat_keyword_or_panic(KW_FUNC);
	Token       func_name_tok = eat_ident_or_panic();
	std::string func_name     = func_name_tok.str_data;

	eat_given_type_or_panic(Token::Type::LeftParen, "(");

	std::vector<uptr<ParamDecl>> params;
	// arg1: int, arg2: int,)
	// arg1: int, arg2: int)
	while (!is_curr_of_type(Token::Type::RightParen))
	{
		auto param_name_token = eat_ident_or_panic();
		auto param_name       = param_name_token.str_data;
		eat_given_type_or_panic(Token::Type::Column, ":");
		auto type = type_expr();
		params.push_back(std::make_unique<ParamDecl>(
		    curr_env,
		    Ident(param_name, param_name_token.range()),
		    std::move(type)));
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
	auto decl        = std::make_unique<FuncDecl>(
        curr_env,
        Ident{func_name_tok.str_data, func_name_tok.range()},
        std::move(params),
        std::move(return_type),
        std::move(body));

	NamedEntityProperties props;
	props.ident_pos     = func_name_tok.range();
	props.available_pos = curr().range();
	add_name_to_curr_env(decl.get(), props);
	return decl;
}
uptr<TypeExpr> Parser::type_expr()
{
	eat_ident_or_panic("type");
	auto type_name_token = prev();
	return std::make_unique<IdentTypeExpr>(
	    curr_env, Ident(type_name_token.str_data, type_name_token.range()));
}
uptr<Stmt> Parser::statement()
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
uptr<Program> Parser::program()
{
	auto root_env_uptr = std::make_unique<Env>(nullptr, logger);
	root_env           = root_env_uptr.get();
	curr_env           = root_env;

	std::vector<uptr<Decl>> vec;
	while (!is_curr_eof())
	{
		vec.push_back(declaration());
	}
	return std::make_unique<Program>(std::move(vec), std::move(root_env_uptr));
}

void Parser::add_name_to_curr_env(Decl                        *decl,
                                  const NamedEntityProperties &props)
{
	uptr<NamedEntity> named_entity = decl->declare(props);
	std::string       name         = named_entity->ident.name;
	curr_env->add(std::move(named_entity));
}

} // namespace protolang
