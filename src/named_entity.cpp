#include "named_entity.h"
#include "typechecker.h"

namespace protolang
{

uptr<Type> NamedFunc::solve_type(TypeChecker *tc) const
{
	std::vector<uptr<Type>> param_types;

	for (auto &&param : this->params)
	{
		param_types.push_back(param.type->type(tc)->clone());
	}

	return std::make_unique<FuncType>(return_type->type(tc)->clone(),
	                                  std::move(param_types));
}

} // namespace protolang