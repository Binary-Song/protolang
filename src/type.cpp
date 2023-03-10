#include "type.h"
#include "env.h"
#include "util.h"
namespace protolang
{

IdentType::IdentType(NamedType *namedType)
    : named_type(namedType)
{}
bool IdentType::equal(const Type *_rhs) const
{
	if (auto rhs = dynamic_cast<const IdentType *>(_rhs))
	{
		return rhs->named_type == this->named_type;
	}
	return false;
}
bool IdentType::can_accept(const Type *arg) const
{
	return equal(arg);
}
std::string IdentType::full_name() const
{
	return named_type->ident.name;
}

bool FuncType::equal(const Type *_rhs) const
{
	if (auto rhs = dynamic_cast<const FuncType *>(_rhs))
	{
		return rhs->return_type->equal(this->return_type.get()) &&
		       all_equal<uptr<Type>>(
		           rhs->param_types,
		           this->param_types,
		           [](const uptr<Type> &a, const uptr<Type> &b)
		           {
			           return a->equal(b.get());
		           });
	}
	return false;
}
bool FuncType::can_accept(const Type *arg) const
{
	return this->equal(arg);
}
std::string FuncType::full_name() const
{
	std::string param_list;
	for (auto &&p : param_types)
	{
		param_list += p->full_name();
		param_list += ",";
	}
	param_list.pop_back();
	return std::format("func({})->{}", param_list, return_type->full_name());
}
} // namespace protolang