#pragma once
#include <string>
#include <vector>
#include "ident.h"
#include "logger.h"
namespace protolang
{

struct ErrorEmptyInput : Error
{
	void print(Logger &logger) const override
	{
		logger.print("Empty input.");
	}
};

struct ErrorRead : Error
{
	u8str path;

	void print(Logger &logger) const override
	{
		logger.print(std::format("Read failure: {}", path));
	}
};

struct ErrorWrite : Error
{
	u8str path;

	void print(Logger &logger) const override
	{
		logger.print(std::format("Write failure: {}", path));
	}
};

struct ErrorNotCallable : Error
{
	SrcRange    callee;
	u8str type;

	void print(Logger &logger) const override
	{
		logger.print(std::format(
		    "Cannot invoke expression of type `{}`.", type));
		logger.print("The following is not callable", callee);
	}
};

struct ErrorMemberNotFound : Error
{
	u8str member;
	u8str type;
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
	u8str init_type;
	u8str var_type;
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
	u8str expected;
	u8str actual;
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
	u8str name;

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
	u8str name;

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
	u8str param_type;
	u8str arg_type;

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
struct IOp;
class OverloadSet;
struct ErrorNoMatchingOverload : Error
{
	SrcRange                 call;
	std::vector<IOp *>       overloads;
	std::vector<u8str> arg_types;

	void print(Logger &logger) const override;
};
struct ErrorMultipleMatchingOverloads : Error
{
	SrcRange                 call;
	std::vector<IOp *>       overloads;
	std::vector<u8str> arg_types;

	void print(Logger &logger) const override;
};

struct ErrorForwardReferencing : Error
{
	u8str name;
	SrcRange    defined_here;
	SrcRange    used_here;

	void print(Logger &logger) const override
	{
		logger << std::format("Use before definition `{}`.",
		                      name);
		logger.print("Used here", used_here);
		logger.print("Defined here", defined_here);
	}
};

struct ErrorUnexpectedNameKind : Error
{
	u8str expected;
	SrcRange    name_range;

	void print(Logger &logger) const override
	{
		logger.print(
		    std::format("Name of `{}` expected here.", expected),
		    name_range);
	}
};

struct ErrorUndefinedName : Error
{
	Ident name;
	void  print(Logger &logger) const override
	{
		logger.print(std::format("Use of undefined name `{}`.",
		                         name.name),
		             name.range);
	}
};

struct ErrorNameRedef : Error
{
	Ident    redefined_here;
	SrcRange defined_here;

	void print(Logger &logger) const override
	{
		logger.print(std::format("Redefinition of name `{}`.",
		                         redefined_here.name),
		             redefined_here.range);
		logger.print("Previously defined here", defined_here);
	}
};

struct ErrorZeroPrefixNotAllowed : Error
{
	SrcRange range;
	void     print(Logger &logger) const override
	{
		logger.print("Numeric literals starting with `0`'s are "
		             "not allowed. "
		             "Use the `0x` prefix to represent octals.",
		             range);
	}
};

struct ErrorUnknownCharacter : Error
{
	SrcRange range;
	void     print(Logger &logger) const override
	{
		logger.print("Unknown character.", range);
	}
};
struct ErrorUnexpectedToken : Error
{
	SrcRange    range;
	u8str expected;
	void        print(Logger &logger) const override
	{
		logger.print(
		    std::format("Unexpected token. {} expected here.",
		                expected),
		    range);
	}
};

struct ErrorInternal : Error
{
	u8str message;

	void print(Logger &logger) const override
	{
		logger.print(std::format("Internal error: {}", message));
	}
};

struct ErrorIncompleteBlockInFunc : Error
{
	u8str name;
	SrcRange    defined_here;

	void print(Logger &logger) const override
	{
		logger.print(
		    std::format(
		        "Incomplete block in function `{}`. "
		        "Possible cause: not all code paths return.",
		        name),
		    defined_here);
	}
};

} // namespace protolang