#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "ident.h"
#include "typedef.h"
#include "util.h"
namespace protolang
{

struct IType;
struct IFuncBody;

// 可以存放到env里的玩意
struct IEntity : virtual IJsonDumper
{
	virtual ~IEntity() = default;
	[[nodiscard]] virtual SrcRange get_src_range() const = 0;
};

// 有类型
struct ITyped : virtual IJsonDumper
{
	virtual ~ITyped()                       = default;
	[[nodiscard]] virtual IType *get_type()const = 0;
};

// 类型
struct IType : IEntity
{
	virtual ~IType() = default;
	[[nodiscard]] virtual bool can_accept(
	    const IType *) const                                = 0;
	[[nodiscard]] virtual bool equal(const IType *) const   = 0;
	[[nodiscard]] virtual std::string get_type_name() const = 0;
	[[nodiscard]] virtual IEntity    *get_member(
	       const std::string &)
	{
		return nullptr;
	}
};

struct IVar : ITyped, IEntity
{
	[[nodiscard]] virtual const Ident &get_ident() const = 0;
};

struct IFuncType : IType
{
	[[nodiscard]] virtual IType *get_return_type() const = 0;
	[[nodiscard]] virtual size_t get_param_count() const = 0;
	[[nodiscard]] virtual IType *get_param_type(
	    size_t) const = 0;
};

struct IFunc : IFuncType, ITyped
{
	[[nodiscard]] virtual IFuncBody *get_body() const = 0;
};

struct IFuncBody
{
	virtual ~IFuncBody() = default;
};

} // namespace protolang