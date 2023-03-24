#include "log.h"
#include "ast.h"
#include "overloadset.h"
namespace protolang
{
static void print_overload_set(Logger                   &logger,
                               const std::vector<IOp *> &set)
{
	int ovl_index = 0;
	for (auto &&overload : set)
	{
		if (auto func = dynamic_cast<ast::FuncDecl *>(overload))
		{
			logger.print(
			    fmt::format(
			        u8"Overload [{}] defined here, typed "
			        "`{}`.",
			        ovl_index,
			        func->get_type_name()),
			    func->range());
		}
		else
		{
			logger << fmt::format(u8"Overload [{}] typed `{}`.",
			                      ovl_index,
			                      overload->get_type_name());
		}
		ovl_index++;
	}
}

static void print_arg_types(Logger                      &logger,
                            const std::vector<StringU8> &types)
{
	int i = 0;
	for (auto &&type : types)
	{
		logger << fmt::format(
		    u8"Argument [{}] is typed `{}`", i, type);
		i++;
	}
}

void ErrorNoMatchingOverload::print(Logger &logger) const
{
	logger.print("None of the overloads match.", call);
	print_arg_types(logger, arg_types);
	print_overload_set(logger, overloads);
}

void ErrorMultipleMatchingOverloads::print(Logger &logger) const
{
	logger.print("The call is ambiguous. There are multiple "
	             "matching overloads.",
	             call);
	print_arg_types(logger, arg_types);
	print_overload_set(logger, overloads);
}
} // namespace protolang