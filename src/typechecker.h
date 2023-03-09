#pragma once
#include "ast.h"
#include "env.h"
namespace protolang
{

class TypeChecker
{
public:

	explicit TypeChecker(Env *root_env)
	    : root_env(root_env)
	{}

	void check(const Program *program);

private:

	Env *root_env;

private:
	NamedFunc* resolve_overload(Ident func, Env *env);
	void annotate_expr(Expr *expr);
};

} // namespace protolang