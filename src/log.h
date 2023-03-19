#pragma once
#include "logger.h"
namespace protolang
{

struct ErrorNotCallable : Error
{
	SrcRange    callee;
	std::string type;

	void print(Logger &logger) const override
	{
		logger.print(std::format(
		    "Cannot invoke expression of type `{}`.", type));
		logger.print("Cannot call this.", callee);
	}
};

struct ErrorMemberNotFound : Error
{
	std::string member;
	std::string type;
	SrcRange    here;

	void print(Logger &logger) const override
	{
		logger << std::format(
		    "Member `{}` not found in type `{}`.", member, type);
		logger.print("Here.", here);
	}
};

struct ErrorNameInThisContextIsAmbiguous : Error
{
	Ident name;

	void print(Logger &logger) const override
	{
		logger << std::format("Name `{}` is ambiguous.",
		                      name.name);
		logger.print("Here.", name.range);
	}
};

struct ErrorVarDeclInitExprTypeMismatch : Error
{
	std::string init_type;
	std::string var_type;
	SrcRange    init_range;
	SrcRange    var_ty_range;

	void print(Logger &logger) const override
	{
		logger << std::format("Cannot initialize variable of "
		                      "type `{}` with expression "
		                      "of type `{}`.",
		                      var_type,
		                      init_type);
		logger.print("Variable type here.", var_ty_range);
		logger.print("Init expression here.", init_range);
	}
};

} // namespace protolang