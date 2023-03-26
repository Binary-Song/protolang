#include <concepts>
#include <fmt/xchar.h>
#include "entity_system.h"
#include "builtin.h"
#include "code_generator.h"
#include "env.h"
#include "util.h"
#include "llvm/IR/Type.h"
namespace protolang
{
StringU8 IFuncType::get_type_name()
{
	StringU8 param_list;
	size_t   param_count = this->get_param_count();
	for (size_t i = 0; i < param_count; i++)
	{
		param_list += get_param_type(i)->get_type_name();
		param_list += u8",";
	}
	param_list.pop_back();
	return fmt::format(u8"func({})->{}",
	                   param_list,
	                   get_return_type()->get_type_name());
}

llvm::Value *IType::cast_implicit(CodeGenerator &g,
                                  llvm::Value   *val,
                                  IType         *type)
{
	if (this->equal(type))
		return val;
	// 在隐式类型转换时，必须提前检查。
	assert(this->can_accept(type));
	return this->cast_inst_no_check(g, val, type);
}

llvm::Value *IType::cast_explicit(CodeGenerator &g,
                                  llvm::Value   *val,
                                  IType         *type)
{
	if (this->equal(type))
		return val;
	assert(this->can_accept_explicit(type));
	return this->cast_inst_no_check(g, val, type);
}

} // namespace protolang