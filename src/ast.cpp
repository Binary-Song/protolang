//
// Created by wps on 2023/3/8.
//
#include "ast.h"
#include "namedobject.h"
#include "env.h"
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
StmtCompound::StmtCompound(uptr<Env> env)
	: env(std::move(env))
{}
} // namespace protolang