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
	bool check_args(NamedFunc *func, const std::vector<IType *> &arg_types);
	NamedFunc *overload_resolution(Env                       *env,
	                               const Ident               &func,
	                               const std::vector<IType *> &arg_types);

	IType *get_builtin_type(const std::string &name)
	{
		return root_env->get_builtin_type(name, this);
	}

private:
	Program *program  = nullptr;
	Env     *root_env = nullptr;
};

} // namespace protolang