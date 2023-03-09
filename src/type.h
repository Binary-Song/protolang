#pragma once
#include <string>
#include "typedef.h"
namespace protolang
{
class NamedType;

enum class TypeType
{
	IdentType
};

struct Type
{
	virtual ~Type()                      = default;
	virtual uptr<Type> clone() const     = 0;
	virtual TypeType   type_type() const = 0;
	virtual bool       is_compatible_with(const Type *argt) const;
};

struct IdentType : public Type
{
	NamedType *named_type = nullptr;
	explicit IdentType(NamedType *namedType);
	uptr<Type> clone() const override
	{
		return std::make_unique<IdentType>(named_type);
	}
	TypeType type_type() const override { return TypeType::IdentType; }
	bool     is_compatible_with(const Type *arg) const override;
};

} // namespace protolang