#pragma once
#include "ident.h"
#include "logger.h"
#include <vector>
#include <string>
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
		logger.print("The following is not callable", callee);
	}
};

struct ErrorMemberNotFound : Error
{
	std::string member;
	std::string type;
	SrcRange    used_here;

	void print(Logger &logger) const override
	{
		logger << std::format(
		    "Member `{}` not found in type `{}`.", member, type);
		logger.print("Used here", used_here);
	}
};

struct ErrorNameInThisContextIsAmbiguous : Error
{
	Ident name;

	void print(Logger &logger) const override
	{
		logger << std::format("Name `{}` is ambiguous.",
		                      name.name);
		logger.print("Used here", name.range);
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
		logger.print("Variable type here", var_ty_range);
		logger.print("Init expression here", init_range);
	}
};

struct ErrorReturnTypeMismatch : Error
{
	std::string expected;
	std::string actual;
	SrcRange    return_range;

	void print(Logger &logger) const override
	{
		logger << std::format(
		    "Return type mismatch. Expected `{}`. Got `{}`.",
		    expected,
		    actual);
		logger.print("Returned here", return_range);
	}
};

struct ErrorMissingRightParen : Error
{
	SrcRange left;

	void print(Logger &logger) const override
	{
		logger << std::format("Parenthesis Mismatch. Right "
		                      "parenthesis is missing.");
		logger.print("Left parenthesis here", left);
	}
};

struct ErrorMissingLeftParen : Error
{
	SrcRange right;

	void print(Logger &logger) const override
	{
		logger << std::format("Parenthesis Mismatch. Left "
		                      "parenthesis is missing.");
		logger.print("Right parenthesis here", right);
	}
};

struct ErrorExpressionExpected : Error
{
	SrcRange curr;
	void     print(Logger &logger) const override
	{
		logger << std::format("Expression expected.");
		logger.print("Here", curr);
	}
};

struct ErrorDeclExpected : Error
{
	SrcRange curr;

	void print(Logger &logger) const override
	{
		logger << std::format("Declaration expected");
		logger.print("Here", curr);
	}
};

struct ErrorFunctionAlreadyExists : Error
{
	std::string name;

	void print(Logger &logger) const override
	{
		logger << std::format("During codegen, `{}`, the "
		                      "function being generated, "
		                      "already has a definition",
		                      name);
	}
};

struct ErrorMissingFunc : Error
{
	std::string name;

	void print(Logger &logger) const override
	{
		logger << std::format("During codegen, `{}`, the called "
		                      "function does not exist.",
		                      name);
	}
};

struct ErrorCallTypeMismatch : Error
{
	SrcRange    call;
	size_t      arg_index;
	std::string param_type;
	std::string arg_type;

	void print(Logger &logger) const override
	{
		logger << "Type mismatch.";
		logger.print(
		    std::format(
		        "In the call below, argument [{}] has type "
		        "`{}`, which "
		        "is incompatible with parameter type `{}`.",
		        arg_index,
		        arg_type,
		        param_type),
		    call);
	}
};

struct ErrorCallArgCountMismatch : Error
{
	SrcRange call;
	size_t   required;
	size_t   provided;

	void print(Logger &logger) const override
	{
		logger.print(
		    std::format("Incorrect number of arguments. "
		                "Required {}, {} provided.",
		                required,
		                provided),
		    call);
	}
};
class OverloadSet;
struct ErrorNoMatchingOverload : Error
{
	SrcRange                 call;
	OverloadSet             *overloads = nullptr;
	std::vector<std::string> arg_types;

	void print(Logger &logger) const override;
};

} // namespace protolang