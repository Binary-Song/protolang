#include "ast.h"

#include <utility>
#include "entity_system.h"
#include "env.h"
#include "named_entity.h"
namespace protolang
{
namespace ast
{

// === IdentTypeExpr ===
IdentTypeExpr::IdentTypeExpr(Env *env, Ident ident)
    : m_ident(std::move(ident))
    , m_env(env)
{}
const IType *IdentTypeExpr::get_type() const
{
	return this->env()->get_one<IType>(ident());
}

// === BinaryExpr ===
const IType *BinaryExpr::get_type() const
{
	auto   lhs_type = this->left->get_type();
	auto   rhs_type = this->right->get_type();
	IFunc *func =
	    env()->overload_resolution(op, {lhs_type, rhs_type});
	return func->get_return_type();
}

// === UnaryExpr ===
const IType *UnaryExpr::get_type() const
{
	auto   operand_type = operand->get_type();
	IFunc *func = env()->overload_resolution(op, {operand_type});
	return func->get_return_type();
}

// === CallExpr ===
std::vector<const IType *> CallExpr::arg_types() const
{
	std::vector<const IType *> result;
	for (auto &&arg : m_args)
	{
		result.push_back(arg->get_type());
	}
	return result;
}

const IType *CallExpr::get_type() const
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
		IFunc *func = env()->overload_resolution(
		    ident_expr->ident(), arg_types());
		return func->get_return_type();
	}
	// 否则，如果是函数指针等，不用重载决策直接调用
	else if (auto func_type = dynamic_cast<const IFunc *>(
	             m_callee->get_type()))
	{
		// 检查参数类型
		env()->check_args(func_type, arg_types(), true);
		return func_type->get_return_type();
	}
	else
	{
		ErrorNotCallable e;
		e.actual_type = m_callee->get_type()->get_type_name();
		e.code_refs.push_back(
		    CodeRef(m_callee->range(), "callee:"));
		env()->logger.log(e);
		throw ExceptionPanic();
	}
}

// === MemberAccessExpr ===
const IType *MemberAccessExpr::get_type() const
{
	auto member_entity =
	    m_left->get_type()->get_member(m_member.name);
	if (member_entity)
	{
		if (auto member_func =
		        dynamic_cast<const IFunc *>(member_entity))
		{
			// todo : member func 的 type 应该不一样！但我建议
			// 不要在这里改，在 IFunc 的实现类改！
			return member_func->get_type();
		}
		else if (auto member_var =
		             dynamic_cast<const IVar *>(member_entity))
		{
			return member_var->get_type();
		}
	}
	env()->logger.log(
	    ErrorNoMember(member_entity->get_src_range(),
	                  m_left->get_type()->get_type_name()));
	throw ExceptionPanic();
}

// === LiteralExpr ===
const IType *LiteralExpr::get_type() const
{
	if (m_token.type == Token::Type::Int)
	{
		return root_env()->get_one<IType>(
		    Ident("int", m_token.range()));
	}
	else if (m_token.type == Token::Type::Fp)
	{
		return root_env()->get_one<IType>(
		    Ident("double", m_token.range()));
	}
	assert(false); // 没有这种literal
	return nullptr;
}

// === IdentExpr ===
const IType *IdentExpr::get_type() const
{
	auto member_entity = env()->get_one(m_ident);
	if (member_entity)
	{
		if (auto member_func =
		        dynamic_cast<IFunc *>(member_entity))
		{
			return member_func->get_type();
		}
		else if (auto member_var =
		             dynamic_cast<IVar *>(member_entity))
		{
			return member_var->get_type();
		}
	}
	env()->logger.log(
	    ErrorSymbolKindIncorrect(m_ident, "non-type", "type"));
	throw ExceptionPanic();
}

Block::Block(Env                   *outer_env,
             SrcRange               range,
             std::vector<uptr<Ast>> elems)
    : m_elems(std::move(elems))
    , m_range(range)
    , m_outer_env(outer_env)
{
	m_inner_env = Env::create(outer_env, outer_env->logger);
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
                       Ident ident,
                       uptr<StructBody> body)
    : m_env(env)
    , m_range(range)
    , m_ident(std::move(ident))
    , m_body(std::move(body))
{}

Env *Ast::root_env() const
{
	return env()->get_root();
}

Program::Program(std::vector<uptr<Decl>> decls, Env *root_env)
    : decls(std::move(decls))
    , root_env(root_env)
{}
} // namespace ast
} // namespace protolang