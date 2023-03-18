#include <concepts>
#include "entity_system.h"
#include "builtin.h"
#include "code_generator.h"
#include "env.h"
#include "util.h"
#include "llvm/IR/Type.h"
namespace protolang
{
std::string IFuncType::get_type_name()
{
	std::string param_list;
	size_t      param_count = this->get_param_count();
	for (size_t i = 0; i < param_count; i++)
	{
		param_list += get_param_type(i)->get_type_name();
		param_list += ",";
	}
	param_list.pop_back();
	return std::format("func({})->{}",
	                   param_list,
	                   get_return_type()->get_type_name());
}

llvm::Value *IType::cast_implicit(CodeGenerator &g,
                                  llvm::Value   *val,
                                  IType         *type)
{
	if (type->equal(this))
		return val;
	if (!type->can_accept(this))
		return nullptr;
	return this->cast_inst_no_check(g, val, type);
}

llvm::Value *IType::cast_explicit(CodeGenerator &g,
                                  llvm::Value   *val,
                                  IType         *type)
{
	if (type->equal(this))
		return val;
	return this->cast_inst_no_check(g, val, type);
}


} // namespace protolang