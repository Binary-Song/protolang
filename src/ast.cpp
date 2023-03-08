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
	return uptr<NamedEntity>(new NamedVar(props, name, type.get()));
}
uptr<NamedEntity> ParamDecl::declare(const NamedEntity::Properties &props) const
{
	return uptr<NamedEntity>(new NamedVar(props, name, type.get()));
}
uptr<NamedEntity> FuncDecl::declare(const NamedEntity::Properties &props) const
{
	std::vector<NamedVar> func_params;
	for (auto &&param : params)
	{
		func_params.emplace_back(props, param->name, param->type.get());
	}
	return std::make_unique<NamedFunc>(
	    props, name, return_type.get(), std::move(func_params));
}

FuncDecl::FuncDecl(std::string                  name,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : Decl(std::move(name))
    , params(std::move(params))
    , return_type(std::move(return_type))
    , body(std::move(body))
{}

CompoundStmt::CompoundStmt(uptr<Env> env)
    : env(std::move(env))
{}

} // namespace protolang