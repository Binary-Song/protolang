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

NamedFunc *TypeChecker::resolve_overload(Ident func_ident, Env *env)
{
	auto overload_set = env->get(func_ident);

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

uptr<Type> BinaryExpr::eval_type(TypeChecker *tc)
{
	auto lhs_type = this->left->eval_type(tc);
	auto rhs_type = this->right->eval_type(tc);
	NamedFunc * func = tc->resolve_overload(this->op, env);

}

} // namespace protolang
