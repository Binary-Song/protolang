#include <utility>
#include "ast.h"
#include "entity_system.h"
#include "env.h"
#include "log.h"
#include "logger.h"

namespace protolang::ast
{

// === IdentTypeExpr ===
IdentTypeExpr::IdentTypeExpr(Env *env, Ident ident)
    : m_ident(std::move(ident))
    , m_env(env)
{}

IType *IdentTypeExpr::get_type()
{
	return lazy_get_type();
}
IType *IdentTypeExpr::recompute_type()
{
	return this->env()->get<IType>(ident());
}

void IdentTypeExpr::validate_types()
{
	[[maybe_unused]] auto _ = get_type();
}

// === BinaryExpr ===
IType *BinaryExpr::get_type()
{
	return lazy_get_type();
}
IType *BinaryExpr::recompute_type()
{
	auto lhs_type = this->left->get_type();
	auto rhs_type = this->right->get_type();
	IOp *func =
	    env()->overload_resolution(op, {lhs_type, rhs_type});
	return func->get_return_type();
}

// === UnaryExpr ===
IType *UnaryExpr::get_type()
{
	auto operand_type = operand->get_type();
	IOp *func = env()->overload_resolution(op, {operand_type});
	return func->get_return_type();
}

// === CallExpr ===
IType *CallExpr::get_arg_type(size_t index)
{
	return m_args[index]->get_type();
}
IType *CallExpr::get_type()
{
	return lazy_get_type();
}
IType *CallExpr::recompute_type()
{
	// non-member call: func(1,2,3)
	// member call:
	// - a.func(1,2,3)
	// - (((a.b).c).func)(1,2,3)
	// 如何判断是不是调用成员函数？
	// 1. 看callee是不是MemberAccessExpr
	// 2.
	// 如果是，看member是不是成员函数类型（而不是成员函数指针等callable对象）
	// 3. 如果是，那没跑了，直接按成员函数法调用。
	// todo: 实现成员函数
	// 另外，如果callee是个名字，则执行overload resolution，
	// 如果callee是个表达式，则用不着执行overload resolution.

	// 目前没有成员函数，这是调用自由函数的情况（进行重载决策）
	if (auto ident_expr =
	        dynamic_cast<IdentExpr *>(m_callee.get()))
	{
		IOp *func = env()->overload_resolution(
		    ident_expr->ident(), get_arg_types());
		auto return_type = func->get_return_type();
		auto func_type   = func->get_type();
		ident_expr->set_type(func_type);
		return return_type;
	}
	// 否则，如果是函数指针等，不用重载决策直接调用
	else if (auto func_type =
	             dynamic_cast<IOp *>(m_callee->get_type()))
	{
		// 检查参数类型
		try
		{
			env()->check_args(
			    func_type, get_arg_types(), true, false);
		}
		catch (ErrorCallTypeMismatch &e)
		{
			e.call = this->range();
			throw e;
		}
		catch (ErrorCallArgCountMismatch &e)
		{
			e.call = this->range();
			throw e;
		}
		return func_type->get_return_type();
	}
	else
	{
		ErrorNotCallable e;
		e.callee = m_callee->range();
		e.type   = m_callee->get_type()->get_type_name();
		throw std::move(e);
	}
}

// === MemberAccessExpr ===
IType *MemberAccessExpr::get_type()
{
	return lazy_get_type();
}
IType *MemberAccessExpr::recompute_type()
{
	auto member_entity =
	    m_left->get_type()->get_member(m_member);
	if (member_entity)
	{
		if (auto member_func =
		        dynamic_cast<IOp *>(member_entity))
		{
			// todo : member func 的 type 应该不一样！但我建议
			// 不要在这里改，在 IFunc 的实现类改！
			return member_func->get_type();
		}
		else if (auto member_var =
		             dynamic_cast<IVar *>(member_entity))
		{
			return member_var->get_type();
		}
	}
	ErrorMemberNotFound e;
	e.type      = m_left->get_type()->get_type_name();
	e.member    = m_member.name;
	e.used_here = m_member.range;
	throw std::move(e);
}
llvm::Value *MemberAccessExpr::codegen_value(
    [[maybe_unused]] CodeGenerator &g)
{
	throw ExceptionNotImplemented{};
}

// === LiteralExpr ===
IType *LiteralExpr::get_type()
{
	return lazy_get_type();
}

IType *LiteralExpr::recompute_type()
{
	if (m_token.type == Token::Type::Int)
	{
		return root_env()->get<IType>(
		    Ident("int", m_token.range()));
	}
	else if (m_token.type == Token::Type::Fp)
	{
		return root_env()->get<IType>(
		    Ident("double", m_token.range()));
	}
	assert(false); // 没有这种literal
	return nullptr;
}

// === IdentExpr ===
IType *IdentExpr::get_type()
{
	return lazy_get_type();
}
IType *IdentExpr::recompute_type()
{
	auto entity = env()->get(ident());
	if (dynamic_cast<OverloadSet *>(entity))
	{
		// 如果标识符是重载集合，则需要上层表达式来帮忙决定type
		ErrorNameInThisContextIsAmbiguous e;
		e.name = this->ident();
		throw std::move(e);
	}
	if (auto typed = dynamic_cast<ITyped *>(entity))
	{
		return typed->get_type();
	}
	ErrorNameInThisContextIsAmbiguous e;
	e.name = this->ident();
	throw std::move(e);
}

void IdentExpr::set_type(IType *t)
{
	set_type_cache(t);
}

// === Block ===

void VarDecl::validate_types()
{
	assert(m_init || m_type);

	if (!m_type)
	{
		// 类型推断
	}

	auto init_type = m_init->get_type();
	auto var_type  = m_type->get_type();
	if (!var_type->can_accept(init_type))
	{
		ErrorVarDeclInitExprTypeMismatch e;
		e.init_type    = init_type->get_type_name();
		e.var_type     = var_type->get_type_name();
		e.init_range   = m_init->range();
		e.var_ty_range = m_type->range();
		throw std::move(e);
	}
}

FuncDecl::FuncDecl(Env                         *env,
                   SrcRange                     range,
                   Ident                        ident,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : m_env(env)
    , m_range(range)
    , m_ident(std::move(ident))
    , m_params(std::move(params))
    , m_return_type(std::move(return_type))
    , m_body(std::move(body))
{}

StructDecl::StructDecl(Env             *env,
                       const SrcRange  &range,
                       Ident            ident,
                       uptr<StructBody> body)
    : m_env(env)
    , m_range(range)
    , m_ident(std::move(ident))
    , m_body(std::move(body))
{}

bool StructDecl::can_accept(IType *other)
{
	return this->equal(other);
}

bool StructDecl::equal(IType *other)
{
	return this == dynamic_cast<const StructDecl *>(other);
}

std::string StructDecl::get_type_name()
{
	return m_ident.name;
}

Env *Ast::root_env() const
{
	return env()->get_root();
}

Program::Program(std::vector<uptr<Decl>> decls, Logger &logger)
    : m_decls(std::move(decls))
    , m_root_env(Env::create_root(logger))
{}

// 表达式默认的语义检查方法是计算一次类型

void Expr::validate_types()
{
	[[maybe_unused]] auto _ = get_type();
}

} // namespace protolang::ast