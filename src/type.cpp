#include "type.h"
#include "env.h"
namespace protolang
{

IdentType::IdentType(NamedType *namedType)
    : named_type(namedType)
{}

bool IdentType::is_compatible_with(const Type *argt) const
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

} // namespace protolang