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

	explicit TypeChecker(Logger &logger)
	    : logger(logger)
	{}

	void set_program(Program *_program)
	{
		this->program = _program;
		root_env      = _program->root_env.get();
	}
	void check();
	void add_builtin_facility();

	bool check_args(NamedFunc *func, const std::vector<Type *> &arg_types);
	NamedFunc *overload_resolution(Env                       *env,
	                               const Ident               &func,
	                               const std::vector<Type *> &arg_types);
	Type      *get_builtin_type(const std::string &name);

private:
	Program                              *program;
	Env                                  *root_env;
	std::map<std::string, uptr<TypeExpr>> builtin_type_exprs;

private:
	void add_builtin_op(const std::string &op, IdentTypeExpr *operand_type);
	void add_builtin_type(const std::string &name);
};

} // namespace protolang