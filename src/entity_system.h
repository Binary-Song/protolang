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
};

// 有类型
struct ITyped : virtual IJsonDumper
{
	virtual ~ITyped() = default;
	[[nodiscard]] virtual const IType *get_type() const = 0;
};

// 类型
struct IType : IEntity
{
	virtual ~IType() = default;
	[[nodiscard]] virtual bool can_accept(
	    const IType *) const                                = 0;
	[[nodiscard]] virtual bool equal(const IType *) const   = 0;
	[[nodiscard]] virtual std::string get_type_name() const = 0;
	[[nodiscard]] virtual const IEntity *get_member(
	    const std::string &) const
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
	[[nodiscard]] virtual const IType *get_return_type()
	    const                                            = 0;
	[[nodiscard]] virtual size_t get_param_count() const = 0;
	[[nodiscard]] virtual const IType *get_param_type(
	    size_t) const = 0;

	// === 实现 IType  ===
	[[nodiscard]] bool can_accept(
	    const IType *other) const override
	{
		return this->equal(other);
	}
	[[nodiscard]] bool equal(const IType *other) const override
	{
		if (auto other_func =
		        dynamic_cast<const IFuncType *>(other))
		{
			// 检查参数、返回值类型
			if (this->get_param_count() !=
			    other_func->get_param_count())
				return false;
			if (this->get_return_type()->equal(
			        other_func->get_return_type()))
				return false;
			size_t param_count = get_param_count();
			for (size_t i = 0; i < param_count; i++)
			{
				if (!this->get_param_type(i)->equal(
				        other_func->get_param_type(i)))
					return false;
			}
			return true;
		}
		return false;
	}
	[[nodiscard]] std::string get_type_name() const override;
};

struct IFunc : IFuncType, ITyped
{
	[[nodiscard]] virtual const IFuncBody *get_body() const = 0;

	// 重写 ITyped
	[[nodiscard]] const IType *get_type() const override
	{
		return this;
	}
};

struct IFuncBody
{};

template <typename Data>
struct Cache
{
private:
	mutable uptr<Data> m_cache;
	virtual void reevaluate_cache(uptr<Data> &result) const = 0;

public:
	Data *read_cache() const
	{
		if (m_cache == nullptr)
			reevaluate_cache(m_cache);
		return m_cache.get();
	}
};

} // namespace protolang