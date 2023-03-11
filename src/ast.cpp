//
// Created by wps on 2023/3/8.
//
#include "ast.h"
#include "entity_system.h"
#include "env.h"
#include "named_entity.h"
namespace protolang
{
// === IdentTypeExpr ===
IdentTypeExpr::IdentTypeExpr(Env *env, Ident ident)
    : m_ident(std::move(ident))
    , m_env(env)
{}
IType *IdentTypeExpr::get_type()
{
	return this->env()->get_one<IType>(ident());
}

// === BinaryExpr ===
IType *BinaryExpr::get_type()
{
	auto   lhs_type = this->left->get_type();
	auto   rhs_type = this->right->get_type();
	IFunc *func =
	    env()->overload_resolution(op, {lhs_type, rhs_type});
	return func->get_return_type();
}

// === UnaryExpr ===
IType *UnaryExpr::get_type()
{
	auto   operand_type = operand->get_type();
	IFunc *func = env()->overload_resolution(op, {operand_type});
	return func->get_return_type();
}

// === CallExpr ===
std::vector<IType *> CallExpr::arg_types() const
{
	std::vector<IType *> result;
	for (auto &&arg : args)
	{
		result.push_back(arg->get_type());
	}
	return result;
}
IType *CallExpr::get_type()
{
	IFunc *func = env()->overload_resolution(op, arg_types());
	return func->get_return_type();
}

NamedVar *VarDecl::add_to_env(const NamedEntityProperties &props,
                              Env *env) const
{
	auto box =
	    std::make_unique<NamedVar>(props, ident, type.get());
	auto ref = box.get();
	env->add_non_func(std::move(box));
	return ref;
}

NamedVar *ParamDecl::add_to_env(
    const NamedEntity::Properties &props, Env *env) const
{
	auto box =
	    std::make_unique<NamedVar>(props, ident, type.get());
	auto ref = box.get();
	env->add_non_func(std::move(box));
	return ref;
}

NamedFunc *FuncDecl::add_to_env(
    const NamedEntity::Properties &props, Env *env) const
{
	std::vector<NamedVar *> registered_params;
	for (auto &&param : params)
	{
		// todo: 把available pos 改成一个点，而不是一个范围
		NamedEntityProperties p;
		p.ident_pos     = param->ident.range;
		p.available_pos = this->body->range;
		auto named_param =
		    param->add_to_env(p, this->body->env.get());
		registered_params.emplace_back(named_param);
	}
	auto box =
	    std::make_unique<NamedFunc>(props,
	                                ident,
	                                return_type.get(),
	                                std::move(registered_params),
	                                body.get());
	auto ref = box.get();
	env->add_func(std::move(box));
	return ref;
}

NamedType *StructDecl::add_to_env(
    const NamedEntityProperties &props, Env *env) const
{
	std::vector<NamedEntity *> members;
	for (auto &&decl : this->body->elems)
	{
		if (auto var_decl = decl->var_decl())
		{
			NamedEntityProperties p;
			p.ident_pos     = var_decl->ident.range;
			p.available_pos = this->body->range;

			auto entity =
			    var_decl->add_to_env(p, this->body->env.get());
			members.push_back(entity);
		}
		else if (auto func_decl = decl->func_decl())
		{
			NamedEntityProperties p;
			p.ident_pos     = func_decl->ident.range;
			p.available_pos = this->body->range;

			auto entity =
			    func_decl->add_to_env(p, this->body->env.get());
			members.push_back(entity);
		}
		else
		{
			assert(0); // 没考虑
		}
	}
	auto box = std::make_unique<NamedType>(
	    props, ident, std::move(members));
	auto ref = box.get();
	env->add_non_func(std::move(box));
	return ref;
}

// std::vector<IType *> CallExpr::arg_types(TypeChecker *tc)
// const
//{
//	std::vector<IType *> result;
//	for (auto &&arg : args)
//	{
//		result.push_back(arg->get_type(tc));
//	}
//	return result;
// }

FuncDecl::FuncDecl(Env                         *env,
                   SrcRange                     range,
                   Ident                        name,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : Decl(env, range, std::move(name))
    , params(std::move(params))
    , return_type(std::move(return_type))
    , body(std::move(body))
{}

CompoundStmt::CompoundStmt(
    Env                                *enclosing_env,
    SrcRange                            range,
    uptr<Env>                           _env,
    std::vector<uptr<CompoundStmtElem>> elems)
    : Stmt(enclosing_env, range)
    , env(std::move(_env))
    , elems(std::move(elems))
{}

StructBody::StructBody(Env      *enclosing_env,
                       SrcRange  range,
                       uptr<Env> _env,
                       std::vector<uptr<MemberDecl>> elems)
    : Ast(enclosing_env, range)
    , env(std::move(_env))
    , elems(std::move(elems))
{}

Program::Program(std::vector<uptr<Decl>> decls, Env *root_env)
    : Ast(root_env, {})
    , decls(std::move(decls))
    , root_env(std::move(root_env))
{}
MemberDecl::MemberDecl(uptr<FuncDecl> func_decl)
    : _func_decl(std::move(func_decl))
{}
MemberDecl::MemberDecl(uptr<VarDecl> var_decl)
    : _var_decl(std::move(var_decl))
{}
StructDecl::StructDecl(Env             *env,
                       const SrcRange  &range,
                       const Ident     &ident,
                       uptr<StructBody> body)
    : Decl(env, range, ident)
    , body(std::move(body))
{}
} // namespace protolang