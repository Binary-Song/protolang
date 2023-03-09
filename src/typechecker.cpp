#include "typechecker.h"
#include "type.h"
namespace protolang
{

void TypeChecker::check(const Program *program)
{
	for (auto &&decl : program->decls)
	{
		decl->check_type(this);
	}
}

void TypeChecker::annotate_expr(Expr *expr) {}

bool TypeChecker::check_args(NamedFunc                 *func,
                             const std::vector<Type *> &arg_types)
{
	if (func->params.size() != arg_types.size())
		return false;
	size_t argc = func->params.size();
	for (size_t i = 0; i < argc; i++)
	{
		auto p = func->params[i].type->type(this);
		auto a = arg_types[i];
		if (!p->is_compatible_with(a))
		{
			return false;
		}
	}
	return true;
}

NamedFunc *TypeChecker::overload_resolution(
    Env *env, const Ident &func, const std::vector<Type *> &arg_types)
{
	auto                     overloads = env->get_all(func);
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
		logger.log(ErrorNoMatchingOverload(func));
		throw ExceptionPanic();
	}
	if (fits.size() > 1)
	{
		logger.log(ErrorMultipleMatchingOverload(func));
		throw ExceptionPanic();
	}
	return fits[0];
}
void TypeChecker::add_builtin_types()
{
	uptr<IdentTypeExpr> int_type =
	    std::make_unique<IdentTypeExpr>(root_env, Ident{"int", {}});
	uptr<IdentTypeExpr> double_type =
	    std::make_unique<IdentTypeExpr>(root_env, Ident{"double", {}});

	NamedEntityProperties props;
	NamedFunc(props, Ident("+", {}), int_type.get(), {});

	this->fake_exprs.push_back(std::move(int_type));
	this->fake_exprs.push_back(std::move(double_type));
}

/*
 *
 *             AST 相关虚函数实现
 *
 */

void VarDecl::check_type(TypeChecker *tc)
{
	// 检查 init 和 声明的类别相等
}

uptr<Type> BinaryExpr::solve_type(TypeChecker *tc)
{
	auto       lhs_type = this->left->type(tc);
	auto       rhs_type = this->right->type(tc);
	NamedFunc *func = tc->overload_resolution(env, op, {lhs_type, rhs_type});
	return func->return_type->type(tc)->clone();
}

uptr<Type> IdentTypeExpr::solve_type(TypeChecker *tc)
{
	NamedEntity *ent = this->env->get_one(ident);
	if (ent->named_entity_type() != NamedEntityType::NamedType)
	{
		// 存在这个名字，但根本不是个类型，你把它当类型用是吧TNND
		tc->logger.log(ErrorSymbolIsNotAType(this->ident));
		throw ExceptionPanic();
	}
	// 是type
	NamedType *type = dynamic_cast<NamedType *>(ent);
	return std::make_unique<IdentType>(type);
}

} // namespace protolang
