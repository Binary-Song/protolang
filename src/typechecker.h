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

	void check(const Program *program)
	{
		for (auto &&decl : program->decls)
		{
			decl->check_type(this);
		}
	}

private:

	Env *root_env;

private:
	void check_var(const VarDecl &decl)
	{
		//	if(decl.init->)
	}
	void check_func(const FuncDecl &decl) {}

	void annotate_expr(Expr *expr) {}
};

} // namespace protolang