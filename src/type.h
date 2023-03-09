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
	virtual ~Type()                                        = default;
	virtual uptr<Type>  clone() const                      = 0;
	virtual TypeType    type_type() const                  = 0;
	virtual bool        can_accept(const Type *argt) const = 0;
	virtual std::string full_name() const                  = 0;
};

struct IdentType : public Type
{
public:
	NamedType *named_type = nullptr;

public:
	explicit IdentType(NamedType *namedType);

	uptr<Type> clone() const override
	{
		return std::make_unique<IdentType>(named_type);
	}
	TypeType    type_type() const override { return TypeType::IdentType; }
	bool        can_accept(const Type *arg) const override;
	std::string full_name() const override;
};

} // namespace protolang