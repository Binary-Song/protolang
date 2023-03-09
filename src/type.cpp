#include "type.h"
#include "env.h"
namespace protolang
{

IdentType::IdentType(NamedType *namedType)
    : named_type(namedType)
{}

bool IdentType::can_accept(const Type *argt) const
{
	switch (argt->type_type())
	{
	case TypeType::IdentType: {
		auto *argt_ident = dynamic_cast<const IdentType *>(argt);
		return argt_ident->named_type == this->named_type;
	}
	}
	return false;
}
std::string IdentType::full_name() const
{
	return named_type->ident.name;
}

} // namespace protolang