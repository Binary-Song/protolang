#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "ident.h"
#include "typedef.h"
#include "util.h"

namespace llvm
{
class Type;
class Value;
} // namespace llvm

namespace protolang
{

struct CodeGenerator;
struct IType;
struct IFuncBody;

namespace ast
{
struct Expr;
}

// 可以存放到env里的玩意
struct IEntity : virtual IJsonDumper
{
	virtual ~IEntity() = default;
};

// 有类型
struct ITyped : virtual IJsonDumper
{
	virtual ~ITyped()                       = default;
	[[nodiscard]] virtual IType *get_type() = 0;
};

// 类型
struct IType : IEntity
{
	virtual ~IType() = default;
	[[nodiscard]] virtual bool        can_accept(IType *) = 0;
	[[nodiscard]] virtual bool        equal(IType *)      = 0;
	[[nodiscard]] virtual std::string get_type_name()     = 0;
	[[nodiscard]] virtual IEntity    *get_member(const Ident &)
	{
		return nullptr;
	}
	[[nodiscard]] virtual llvm::Type *get_llvm_type(
	    CodeGenerator &g) const = 0;
};

struct IVar : ITyped, IEntity
{
	[[nodiscard]] virtual Ident get_ident() const = 0;
	virtual ast::Expr          *get_init() const  = 0;
	llvm::Value                *codegen(CodeGenerator &g);
};

struct IFuncType : IType
{
	[[nodiscard]] virtual IType *get_return_type()       = 0;
	[[nodiscard]] virtual size_t get_param_count() const = 0;
	[[nodiscard]] virtual IType *get_param_type(size_t)  = 0;

	// === 实现 IType  ===
	[[nodiscard]] bool can_accept(IType *other) override
	{
		return this->equal(other);
	}
	[[nodiscard]] bool equal(IType *other) override
	{
		if (auto other_func = dynamic_cast<IFuncType *>(other))
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
	[[nodiscard]] std::string get_type_name() override;
};

struct IFunc : IFuncType, ITyped
{
	[[nodiscard]] virtual IFuncBody *get_body() = 0;
	// 重写 ITyped
	[[nodiscard]] IType *get_type() override { return this; }
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