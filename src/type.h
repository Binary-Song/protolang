#pragma once
#include <cassert>
#include <string>
#include <vector>
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
	virtual bool        equal(const Type *other) const     = 0;
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
	bool        equal(const Type *other) const override;
	std::string full_name() const override;
};

struct FuncType : public Type
{
public:
	uptr<Type>              return_type;
	std::vector<uptr<Type>> param_types;

	explicit FuncType(uptr<Type>              return_type,
	                  std::vector<uptr<Type>> param_types)
	    : return_type(std::move(return_type))
	    , param_types(std::move(param_types))
	{}

	uptr<Type> clone() const override
	{
		std::vector<uptr<Type>> clone_param_type;
		for (auto &&pt : this->param_types)
		{
			clone_param_type.push_back(pt->clone());
		}
		return std::make_unique<FuncType>(return_type->clone(),
		                                  std::move(clone_param_type));
	}
	TypeType    type_type() const override { return TypeType::IdentType; }
	bool        equal(const Type *other) const override;
	bool        can_accept(const Type *arg) const override;
	std::string full_name() const override;
};

class TypeChecker;

struct ITyped
{
public:
	virtual ~ITyped() = default;

	Type *get_type(TypeChecker *tc)
	{
		if (get_cached_type() == nullptr)
		{
			get_cached_type() = solve_type(tc);
		}
		assert(get_cached_type());
		return get_cached_type().get();
	}

private:
	virtual uptr<Type>  solve_type(TypeChecker *) const = 0;
	virtual uptr<Type> &get_cached_type()               = 0;
};

} // namespace protolang