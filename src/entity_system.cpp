#include "entity_system.h"
#include "env.h"
#include "util.h"
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


} // namespace protolang