//
// Created by wps on 2023/3/8.
//
#include "ast.h"
#include "env.h"
#include "named_entity.h"
#include "type.h"
namespace protolang
{
Expr::Expr(Env *env)
    : Ast(env)
{}
Expr::~Expr() {}

uptr<NamedEntity> VarDecl::declare(const NamedEntity::Properties &props) const
{
	return uptr<NamedEntity>(new NamedVar(props, ident, type.get()));
}

uptr<NamedEntity> ParamDecl::declare(const NamedEntity::Properties &props) const
{
	return uptr<NamedEntity>(new NamedVar(props, ident, type.get()));
}

uptr<NamedEntity> FuncDecl::declare(const NamedEntity::Properties &props) const
{
	std::vector<NamedVar> func_params;
	for (auto &&param : params)
	{
		func_params.emplace_back(props, param->ident, param->type.get());
	}
	return std::make_unique<NamedFunc>(
	    props, ident, return_type.get(), std::move(func_params));
}

FuncDecl::FuncDecl(Env                         *env,
                   Ident                        name,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : Decl(env, std::move(name))
    , params(std::move(params))
    , return_type(std::move(return_type))
    , body(std::move(body))
{}

CompoundStmt::CompoundStmt(Env *enclosing_env, uptr<Env> _env)
    : Stmt(enclosing_env)
    , env(std::move(_env))
{}
} // namespace protolang