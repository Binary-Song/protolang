#pragma once
#include <fmt/xchar.h>
#include <string>
#include <vector>
#include "encoding.h"
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
	StringU8 path;

	void print(Logger &logger) const override
	{
		logger.print(fmt::format(u8"Read failure: {}", path));
	}
};

struct ErrorWrite : Error
{
	StringU8 path;

	void print(Logger &logger) const override
	{
		logger.print(fmt::format(u8"Write failure: {}", path));
	}
};

struct ErrorNotCallable : Error
{
	SrcRange callee;
	StringU8 type;

	void print(Logger &logger) const override
	{
		logger.print(fmt::format(
		    u8"Cannot invoke expression of type `{}`.", type));
		logger.print(u8"The following is not callable", callee);
	}
};

struct ErrorMemberNotFound : Error
{
	StringU8 member;
	StringU8 type;
	SrcRange used_here;

	void print(Logger &logger) const override
	{
		logger << fmt::format(
		    u8"Member `{}` not found in type `{}`.",
		    member,
		    type);
		logger.print("Used here", used_here);
	}
};

struct ErrorNameInThisContextIsAmbiguous : Error
{
	Ident name;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"Name `{}` is ambiguous.",
		                      name.name);
		logger.print("Used here", name.range);
	}
};

struct ErrorVarDeclInitExprTypeMismatch : Error
{
	StringU8 init_type;
	StringU8 var_type;
	SrcRange init_range;
	SrcRange var_ty_range;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"Cannot initialize variable of "
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
	StringU8 expected;
	StringU8 actual;
	SrcRange return_range;

	void print(Logger &logger) const override
	{
		logger << fmt::format(
		    u8"Return type mismatch. Expected `{}`. Got `{}`.",
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
		logger << fmt::format(u8"Parenthesis Mismatch. Right "
		                      "parenthesis is missing.");
		logger.print("Left parenthesis here", left);
	}
};

struct ErrorMissingLeftParen : Error
{
	SrcRange right;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"Parenthesis Mismatch. Left "
		                      "parenthesis is missing.");
		logger.print("Right parenthesis here", right);
	}
};

struct ErrorExpressionExpected : Error
{
	SrcRange curr;
	void     print(Logger &logger) const override
	{
		logger << fmt::format(u8"Expression expected.");
		logger.print("Here", curr);
	}
};

struct ErrorDeclExpected : Error
{
	SrcRange curr;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"Declaration expected");
		logger.print("Here", curr);
	}
};

struct ErrorFunctionAlreadyExists : Error
{
	StringU8 name;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"During codegen, `{}`, the "
		                      "function being generated, "
		                      "already has a definition",
		                      name);
	}
};

struct ErrorMissingFunc : Error
{
	StringU8 name;

	void print(Logger &logger) const override
	{
		logger << fmt::format(
		    u8"During codegen, `{}`, the called "
		    "function does not exist.",
		    name);
	}
};

struct ErrorCallTypeMismatch : Error
{
	SrcRange call;
	size_t   arg_index;
	StringU8 param_type;
	StringU8 arg_type;

	void print(Logger &logger) const override
	{
		logger << "Type mismatch.";
		logger.print(
		    fmt::format(
		        u8"In the call below, argument [{}] has type "
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
		    fmt::format(u8"Incorrect number of arguments. "
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
	SrcRange              call;
	std::vector<IOp *>    overloads;
	std::vector<StringU8> arg_types;

	void print(Logger &logger) const override;
};
struct ErrorMultipleMatchingOverloads : Error
{
	SrcRange              call;
	std::vector<IOp *>    overloads;
	std::vector<StringU8> arg_types;

	void print(Logger &logger) const override;
};

struct ErrorForwardReferencing : Error
{
	StringU8 name;
	SrcRange defined_here;
	SrcRange used_here;

	void print(Logger &logger) const override
	{
		logger << fmt::format(u8"Use before definition `{}`.",
		                      name);
		logger.print("Used here", used_here);
		logger.print("Defined here", defined_here);
	}
};

struct ErrorUnexpectedNameKind : Error
{
	StringU8 expected;
	SrcRange name_range;

	void print(Logger &logger) const override
	{
		logger.print(fmt::format(u8"Name of `{}` expected here.",
		                         expected),
		             name_range);
	}
};

struct ErrorUndefinedName : Error
{
	Ident name;
	void  print(Logger &logger) const override
	{
		logger.print(fmt::format(u8"Use of undefined name `{}`.",
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
		logger.print(fmt::format(u8"Redefinition of name `{}`.",
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
	SrcRange range;
	StringU8 expected;
	void     print(Logger &logger) const override
	{
		logger.print(
		    fmt::format(u8"Unexpected token. {} expected here.",
		                expected),
		    range);
	}
};

struct ErrorInternal : Error
{
	StringU8 message;

	void print(Logger &logger) const override
	{
		logger.print(
		    fmt::format(u8"Internal error: {}", message));
	}
};

struct ErrorIncompleteBlockInFunc : Error
{
	StringU8 name;
	SrcRange defined_here;

	void print(Logger &logger) const override
	{
		logger.print(
		    fmt::format(
		        u8"Incomplete block in function `{}`. "
		        "Possible cause: not all code paths return.",
		        name),
		    defined_here);
	}
};

} // namespace protolang