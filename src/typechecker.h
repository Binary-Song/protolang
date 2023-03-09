#pragma once
#include "ast.h"
#include "env.h"
#include "logger.h"
namespace protolang
{

class TypeChecker
{
public:
	Logger &logger;

	explicit TypeChecker(Env *root_env, Logger &logger)
	    : root_env(root_env)
	    , logger(logger)
	{}

	void check(const Program *program);
	void add_builtin_types( );

	bool check_args(NamedFunc *func, const std::vector<Type *> &arg_types);
	NamedFunc *overload_resolution(Env                       *env,
	                               const Ident               &func,
	                               const std::vector<Type *> &arg_types);
	void       annotate_expr(Expr *expr);

private:
	Env *root_env;
	std::vector<uptr<TypeExpr>> fake_exprs;
};

} // namespace protolang