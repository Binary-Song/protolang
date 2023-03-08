//
// Created by wps on 2023/3/8.
//
#include "ast.h"
#include "env.h"
#include "namedobject.h"
namespace protolang
{
uptr<NamedObject> DeclVar::declare(const NamedObject::Properties &props) const
{
	return uptr<NamedObject>(new NamedVar(props, name, type.get()));
}
uptr<NamedObject> DeclParam::declare(const NamedObject::Properties &props) const
{
	return uptr<NamedObject>(new NamedVar(props, name, type.get()));
}
uptr<NamedObject> DeclFunc::declare(const NamedObject::Properties &props) const
{
	std::vector<NamedVar> func_params;
	for (auto &&param : params)
	{
		func_params.emplace_back(props, param->name, param->type.get());
	}
	return std::make_unique<NamedFunc>(
	    props, name, return_type.get(), std::move(func_params));
}

DeclFunc::DeclFunc(std::string                  name,
                   std::vector<uptr<DeclParam>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<StmtCompound>           body)
    : Decl(std::move(name))
    , params(std::move(params))
    , return_type(std::move(return_type))
    , body(std::move(body))
{}

StmtCompound::StmtCompound(uptr<Env> env)
    : env(std::move(env))
{}

std::string TypeExprIdent::id() const
{
	return name;
}

} // namespace protolang