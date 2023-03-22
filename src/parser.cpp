#include "parser.h"
#include "ast.h"
#include "entity_system.h"
#include "env.h"
#include "exceptions.h"
#include "log.h"
namespace protolang
{

uptr<ast::VarDecl> Parser::var_decl()
{
	// var a : int = 2;
	auto var_token  = eat_keyword_or_panic(Keyword::KW_VAR);
	auto name_token = eat_ident_or_panic();
	auto name       = name_token.str_data;
	uptr<ast::TypeExpr> type;
	uptr<ast::Expr>     init;
	// 类型标注可选

	bool has_type_anno =
	    eat_if_is_given_type({Token::Type::Column});
	if (has_type_anno)
	{
		type = type_expr();
	}
	if (!has_type_anno)
	{
		eat_op_or_panic("="); // 没有类型注释，就必须有初始化
		init = expression();
	}
	else
	{
		// 否则init可选
		if (eat_if_is_given_op({"="}))
		{
			init = expression();
		}
	}

	eat_given_type_or_panic(Token::Type::SemiColumn, ";");

	// 产生符号表记录
	Ident var_ident = Ident(name, name_token.range());
	auto  decl      = std::make_unique<ast::VarDecl>(
        var_ident, std::move(type), std::move(init));
	curr_env->add(var_ident, decl.get());
	return decl;
}

uptr<ast::ExprStmt> Parser::expression_statement()
{
	auto expr = expression();
	auto semi_col =
	    eat_given_type_or_panic(Token::Type::SemiColumn, ";");
	auto range = expr->range() + semi_col.range();

	return make_uptr(new ast::ExprStmt(range, std::move(expr)));
}
uptr<ast::ReturnStmt> Parser::return_statement()
{
	auto return_kw   = eat_keyword_or_panic(Keyword::KW_RETURN);
	bool return_void = is_curr_of_type(Token::Type::SemiColumn);
	auto expr        = !return_void ? expression() : nullptr;
	auto semi_col =
	    eat_given_type_or_panic(Token::Type::SemiColumn, ";");
	auto range = return_kw.range() + semi_col.range();

	if (expr)
		return make_uptr(
		    new ast::ReturnStmt(range, std::move(expr)));
	return uptr<ast::ReturnStmt>(
	    new ast::ReturnVoidStmt(range, curr_env));
}

void Parser::parse_block(
    ast::IBlock                       *block,
    std::function<void(ast::IBlock *)> elem_handler)
{
	EnvGuard guard(curr_env, block->get_inner_env());
	auto     left_brace =
	    eat_given_type_or_panic(Token::Type::LeftBrace, "{");
	while (!is_curr_of_type(Token::Type::RightBrace))
	{
		elem_handler(block);
	}
	auto right_brace =
	    eat_given_type_or_panic(Token::Type::RightBrace, "}");
	block->set_range(
	    range_union(left_brace.range(), right_brace.range()));
}

uptr<ast::CompoundStmt> Parser::compound_statement()
{
	auto compound =
	    ast::IBlock::create_with_inner_env<ast::CompoundStmt>(
	        curr_env);
	parse_block(compound.get(),
	            [this](ast::IBlock *b)
	            {
		            if (is_curr_keyword(Keyword::KW_VAR))
			            b->add_content(var_decl());
		            else
			            b->add_content(statement());
	            });
	return compound;
}

uptr<ast::StructBody> Parser::struct_body()
{
	return {};
	//	return block<ast::StructBody>(
	//	    [this](std::unique_ptr<ast::StructBody> &b)
	//	    {
	//		    if (is_curr_keyword(Keyword::KW_VAR))
	//			    b->add_elem(var_decl());
	//		    else
	//			    b->add_elem(func_decl());
	//	    });
	//
	//	auto struct_body =
	//	    ast::IBlock::create_with_inner_env<ast::StructBody>(
	//	        curr_env);
	//	parse_block(compound.get(),
	//	            [this](ast::IBlock *b)
	//	            {
	//		            if (is_curr_keyword(Keyword::KW_VAR))
	//			            b->add_content(var_decl());
	//		            else
	//			            b->add_content(statement());
	//	            });
	//	return compound;
}

uptr<ast::Expr> Parser::expression()
{
	return assignment();
}
uptr<ast::Expr> Parser::assignment()
{
	uptr<ast::Expr> left = equality();
	if (eat_if_is_given_op({"="}))
	{
		Token           op    = prev();
		uptr<ast::Expr> right = assignment();
		return make_uptr(
		    new ast::BinaryExpr(std::move(left),
		                        Ident(op.str_data, op.range()),
		                        std::move(right)));
	}
	return left;
}
uptr<ast::Expr> Parser::equality()
{
	uptr<ast::Expr> expr = comparison();
	while (eat_if_is_given_op({"==", "!="}))
	{
		Token           op    = prev();
		uptr<ast::Expr> right = comparison();
		expr                  = uptr<ast::Expr>(
            new ast::BinaryExpr(std::move(expr),
                                Ident(op.str_data, op.range()),
                                std::move(right)));
	}
	return expr;
}
uptr<ast::Expr> Parser::comparison()
{
	uptr<ast::Expr> expr = term();
	while (eat_if_is_given_op({">", ">=", "<", "<="}))
	{
		Token           op    = prev();
		uptr<ast::Expr> right = term();
		expr                  = uptr<ast::Expr>(
            new ast::BinaryExpr(std::move(expr),
                                Ident(op.str_data, op.range()),
                                std::move(right)));
	}
	return expr;
}
uptr<ast::Expr> Parser::term()
{
	uptr<ast::Expr> expr = factor();
	while (eat_if_is_given_op({"+", "-"}))
	{
		Token           op    = prev();
		uptr<ast::Expr> right = factor();
		expr                  = uptr<ast::Expr>(
            new ast::BinaryExpr(std::move(expr),
                                Ident(op.str_data, op.range()),
                                std::move(right)));
	}
	return expr;
}
uptr<ast::Expr> Parser::factor()
{
	uptr<ast::Expr> expr = unary_pre();
	while (eat_if_is_given_op({"*", "/", "%"}))
	{
		Token           op    = prev();
		uptr<ast::Expr> right = unary_pre();
		expr                  = uptr<ast::Expr>(
            new ast::BinaryExpr(std::move(expr),
                                Ident(op.str_data, op.range()),
                                std::move(right)));
	}
	return expr;
}
uptr<ast::Expr> Parser::unary_pre()
{
	if (eat_if_is_given_op({"!", "-"}))
	{
		auto            op    = prev();
		uptr<ast::Expr> right = unary_pre();
		return uptr<ast::Expr>(
		    new ast::UnaryExpr(true,
		                       std::move(right),
		                       Ident(op.str_data, op.range())));
	}
	else
	{
		return unary_post();
	}
}
uptr<ast::Expr> Parser::unary_post()
{
	uptr<ast::Expr> lhs = member_access();
	// matrix[1][2](arg1, arg2)
	while (eat_if_is_given_type(
	    {Token::Type::LeftParen, Token::Type::LeftBracket}))
	{
		bool isCall = (prev().type == Token::Type::LeftParen);
		Token::Type rightDelim = isCall
		                             ? Token::Type::RightParen
		                             : Token::Type::RightBracket;

		std::vector<uptr<ast::Expr>> args;
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
		auto right_bound = eat_given_type_or_panic(
		    rightDelim, isCall ? ")" : "]");

		auto range =
		    range_union(lhs->range(), right_bound.range());
		if (isCall)
		{
			lhs = uptr<ast::Expr>(new ast::CallExpr(
			    range, std::move(lhs), std::move(args)));
		}
		else
		{
			lhs = uptr<ast::Expr>(new ast::BracketExpr(
			    range, std::move(lhs), std::move(args)));
		}
	}
	return lhs;
}
uptr<ast::Expr> Parser::member_access()
{
	uptr<ast::Expr> expr = primary();
	while (eat_if_is_given_op({"."}))
	{
		auto op = prev();
		auto id = eat_ident_or_panic();
		expr    = uptr<ast::Expr>(new ast::MemberAccessExpr(
            std::move(expr), Ident{id.str_data, id.range()}));
	}
	return expr;
}
uptr<ast::Expr> Parser::primary()
{
	if (eat_if_is_given_type({Token::Type::Id}))
	{
		return uptr<ast::Expr>(new ast::IdentExpr(
		    curr_env, Ident(prev().str_data, prev().range())));
	}
	if (eat_if_is_given_type({Token::Type::Str,
	                          Token::Type::Int,
	                          Token::Type::Fp}))
	{
		return uptr<ast::Expr>(
		    new ast::LiteralExpr(curr_env, prev()));
	}
	if (eat_if_is_given_type({Token::Type::LeftParen}))
	{
		Token           left_paren = prev();
		uptr<ast::Expr> expr       = expression();
		if (eat_if_is_given_type({Token::Type::RightParen}))
		{
			return expr;
		}
		else
		{
			ErrorMissingRightParen e;
			e.left = left_paren.range();
			throw std::move(e);
		}
	}
	if (curr().type == Token::Type::RightParen)
	{
		ErrorMissingLeftParen e;
		e.right = curr().range();
		throw std::move(e);
	}
	else
	{
		ErrorExpressionExpected e;
		e.curr = curr().range();
		throw std::move(e);
	}
}
uptr<ast::Decl> Parser::declaration()
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
			else if (is_curr_keyword(Keyword::KW_CLASS))
			{
				return struct_decl();
			}
			ErrorDeclExpected e;
			e.curr = curr().range();
			throw std::move(e);
		}
		catch (const Error &e)
		{
			e.print(logger);
			sync();
		}
	}
	exit(1);
}
uptr<ast::FuncDecl> Parser::func_decl()
{
	// func foo(arg1: int, arg2: int) -> int { ... }
	Token       func_kw_token   = eat_keyword_or_panic(KW_FUNC);
	Token       func_name_token = eat_ident_or_panic();
	std::string func_name       = func_name_token.str_data;
	Ident       func_ident{func_name_token.str_data,
                     func_name_token.range()};

	eat_given_type_or_panic(Token::Type::LeftParen, "(");

	std::vector<uptr<ast::ParamDecl>>                   params;
	// arg1: int, arg2: int,)
	// arg1: int, arg2: int)
	std::vector<std::tuple<Ident, uptr<ast::TypeExpr>>> data;
	while (!is_curr_of_type(Token::Type::RightParen))
	{
		// todo: 注意！！！解决参数在外面的问题
		auto param_name_token = eat_ident_or_panic();
		auto param_name       = param_name_token.str_data;
		eat_given_type_or_panic(Token::Type::Column, ":");
		auto type = type_expr();
		data.push_back(
		    {Ident(param_name, param_name_token.range()),
		     std::move(type)});
		//		params.push_back(std::make_unique<ast::ParamDecl>(
		//		    curr_env,
		//		    Ident(param_name, param_name_token.range()),
		//		    std::move(type)));
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

	// 创建参数，在inner env里！
	for (auto &&[ident, type_expr] : data)
	{
		auto decl = make_uptr(new ast::ParamDecl(
		    body->get_inner_env(), ident, std::move(type_expr)));
		body->get_inner_env()->add(ident, decl.get());
		params.push_back(std::move(decl));
	}

	auto decl = std::make_unique<ast::FuncDecl>(
	    curr_env,
	    // note: range 是函数声明的 range
	    range_union(func_kw_token.range(), return_type->range()),
	    func_ident,
	    std::move(params),
	    std::move(return_type),
	    std::move(body));

	curr_env->add(func_ident, decl.get());
	return decl;
}

uptr<ast::StructDecl> Parser::struct_decl()
{
	throw ExceptionNotImplemented();
	//	auto struct_kw_token   = eat_keyword_or_panic(KW_STRUCT);
	//	auto struct_name_token = eat_ident_or_panic();
	//	auto body              = struct_body();
	//	auto decl              =
	// std::make_unique<ast::StructDecl>(
	//        curr_env,
	//        range_union(struct_kw_token.range(),
	//                    struct_name_token.range()),
	//        Ident{struct_name_token.str_data,
	//              struct_name_token.range()},
	//        std::move(body));
	//	curr_env->add(struct_name_token.str_data, decl.get());
	//	return decl;
}

uptr<ast::TypeExpr> Parser::type_expr()
{
	eat_ident_or_panic("type");
	auto type_name_token = prev();
	return std::make_unique<ast::IdentTypeExpr>(
	    curr_env,
	    Ident(type_name_token.str_data,
	          type_name_token.range()));
}
uptr<ast::Stmt> Parser::statement()
{
	if (is_curr_of_type(Token::Type::LeftBrace))
	{
		return compound_statement();
	}
	else if (is_curr_keyword(KW_RETURN))
	{
		return return_statement();
	}
	else
	{
		return expression_statement();
	}
}
uptr<ast::Program> Parser::program()
{
	curr_env = root_env;
	root_env->add_built_in_facility();
	std::vector<uptr<ast::Decl>> vec;

	while (!is_curr_eof())
	{
		vec.push_back(declaration());
	}

	return make_uptr(new ast::Program(std::move(vec), logger));
}

} // namespace protolang
