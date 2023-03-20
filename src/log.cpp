#include "log.h"
#include "ast.h"
#include "overloadset.h"
namespace protolang
{
static void print_overload_set(Logger &logger, OverloadSet *set)
{
	for (auto &&overload : *set)
	{
		int ovl_index = 0;
		if (auto func = dynamic_cast<ast::FuncDecl *>(overload))
		{
			logger.print(
			    std::format(
			        "Overload [{}] defined here, its type "
			        "being `{}`.",
			        ovl_index,
			        func->get_type_name()),
			    func->range());
		}
		else
		{
			logger << std::format(
			    "Overload [{}] is of type `{}`.",
			    ovl_index,
			    overload->get_type_name());
		}
	}
}

static void print_arg_types(
    Logger &logger, const std::vector<std::string> &types)
{
	int i = 0;
	for (auto &&type : types)
	{
		logger << std::format(
		    "Argument [{}] is of type `{}`", i, type);
	}
}

void ErrorNoMatchingOverload::print(Logger &logger) const
{
	logger.print("None of the overloads match.", call);
	print_arg_types(logger, arg_types);
	print_overload_set(logger, overloads);
}
} // namespace protolang