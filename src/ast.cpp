//
// Created by wps on 2023/3/8.
//
#include "ast.h"
#include "env.h"
#include "named_entity.h"
#include "type.h"
namespace protolang
{
Expr::Expr(Env *env, Pos2DRange range)
    : Ast(env, range)
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
std::vector<Type *> CallExpr::arg_types(TypeChecker *tc) const
{
	std::vector<Type *> result;
	for (auto &&arg : args)
	{
		result.push_back(arg->get_type(tc));
	}
	return result;
}
FuncDecl::FuncDecl(Env                         *env,
                   Pos2DRange                   range,
                   Ident                        name,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : Decl(env, range, std::move(name))
    , params(std::move(params))
    , return_type(std::move(return_type))
    , body(std::move(body))
{}

CompoundStmt::CompoundStmt(Env                                *enclosing_env,
                           Pos2DRange                          range,
                           uptr<Env>                           _env,
                           std::vector<uptr<CompoundStmtElem>> elems)
    : Stmt(enclosing_env, range)
    , env(std::move(_env))
    , elems(std::move(elems))
{}

Program::Program(std::vector<uptr<Decl>> decls, uptr<Env> root_env)
    : Ast(root_env.get(), {})
    , decls(std::move(decls))
    , root_env(std::move(root_env))
{}
} // namespace protolang