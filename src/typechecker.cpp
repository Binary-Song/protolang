#include "typechecker.h"
#include "ast.h"
#include "type.h"

namespace protolang
{

void TypeChecker::check()
{
	for (auto &&decl : program->decls)
	{
		try
		{
			decl->check_type(this);
		}
		catch (const ExceptionPanic &)
		{}
	}
}

bool TypeChecker::check_args(NamedFunc                 *func,
                             const std::vector<Type *> &arg_types)
{
	if (func->params.size() != arg_types.size())
		return false;
	size_t argc = func->params.size();
	for (size_t i = 0; i < argc; i++)
	{
		auto p = func->params[i].get_type(this);
		auto a = arg_types[i];
		if (!p->can_accept(a))
		{
			return false;
		}
	}
	return true;
}

NamedFunc *TypeChecker::overload_resolution(
    Env *env, const Ident &func_ident, const std::vector<Type *> &arg_types)
{
	auto                     overloads = env->get_all(func_ident);
	std::vector<NamedFunc *> fits;
	for (auto &&entity : overloads)
	{
		assert(dynamic_cast<NamedFunc *>(entity));
		NamedFunc *func = dynamic_cast<NamedFunc *>(entity);
		if (check_args(func, arg_types))
		{
			fits.push_back(func);
		}
	}
	if (fits.size() == 0)
	{
		logger.log(ErrorNoMatchingOverload(func_ident));
		throw ExceptionPanic();
	}
	if (fits.size() > 1)
	{
		logger.log(ErrorMultipleMatchingOverload(func_ident));
		throw ExceptionPanic();
	}
	return fits[0];
}

/*
 *
 *             AST 相关虚函数实现
 *
 */

void VarDecl::check_type(TypeChecker *tc)
{
	// 检查 init 和 声明的类别兼容
	if (!this->type->type(tc)->can_accept(this->init->type(tc)))
	{
		tc->logger.log(ErrorTypeMismatch(init->type(tc)->full_name(),
		                                 Pos2DRange{},
		                                 type->type(tc)->full_name(),
		                                 Pos2DRange{}));
	}
}

uptr<Type> BinaryExpr::solve_type(TypeChecker *tc) const
{
	auto       lhs_type = this->left->get_type(tc);
	auto       rhs_type = this->right->get_type(tc);
	NamedFunc *func = tc->overload_resolution(env, op, {lhs_type, rhs_type});
	return func->return_type->get_type(tc)->clone();
}

uptr<Type> UnaryExpr::solve_type(TypeChecker *tc) const
{
	auto       operand_type = this->operand->get_type(tc);
	NamedFunc *func         = tc->overload_resolution(env, op, {operand_type});
	return func->return_type->get_type(tc)->clone();
}

uptr<Type> CallExpr::solve_type(TypeChecker *tc) const
{
	// non-member call: func(1,2,3)
	// member call:
	// - a.func(1,2,3)
	// - (((a.b).c).func)(1,2,3)
	// 如何判断是不是调用成员函数？
	// 1. 看callee是不是MemberAccessExpr
	// 2. 如果是，看member是不是成员函数类型（而不是成员函数指针等callable对象）
	// 3. 如果是，那没跑了，直接按成员函数法调用。
	// todo: 实现成员函数
	// 另外，如果callee是个名字，则执行overload resolution，
	// 如果callee是个表达式，则用不着执行overload resolution.

	if (auto ident_expr = dynamic_cast<IdentExpr *>(callee.get()))
	{
		NamedFunc *func =
		    tc->overload_resolution(env, ident_expr->ident, arg_types());
		return func->return_type->get_type(tc)->clone();
	}
	else
	{
		if (auto func_type = dynamic_cast<FuncType *>(callee->get_type(tc)))
		{
			return func_type->return_type->clone();
		}
		else
		{
			ErrorNotCallable e;
			e.actual_type = callee->get_type(tc)->full_name();
			e.code_refs.push_back(CodeRef(callee->range, "callee:"));
			tc->logger.log(e);
			throw ExceptionPanic();
		}
	}
}

uptr<Type> MemberAccessExpr::solve_type(TypeChecker *) const
{
	NamedType *type = this->env->get_one<NamedType>(ident);
	return std::make_unique<IdentType>(type);
}

uptr<Type> IdentTypeExpr::solve_type(TypeChecker *) const
{
	NamedType *type = this->env->get_one<NamedType>(ident);
	return std::make_unique<IdentType>(type);
}

uptr<Type> LiteralExpr::solve_type(TypeChecker *tc)
{
	if (token.type == Token::Type::Int)
	{
		return tc->get_builtin_type("Int")->clone();
	}
	else if (token.type == Token::Type::Fp)
	{
		return tc->get_builtin_type("Double")->clone();
	}
	assert(false); // 没有这种literal
	return nullptr;
}

uptr<Type> IdentExpr::solve_type(TypeChecker *tc)
{
	NamedEntity *entity = env->get_one(this->ident);
	switch (entity->named_entity_type())
	{
	case NamedEntityType::NamedVar:
		return env->get_one<NamedVar>(ident)->type->type(tc)->clone();
	case NamedEntityType::NamedFunc:
		return env->get_one<NamedFunc>(ident)->type(tc)->clone();
	default:
		env->logger.log(ErrorSymbolKindIncorrect(ident, "non-type", "type"));
		throw ExceptionPanic();
	}
}

} // namespace protolang
