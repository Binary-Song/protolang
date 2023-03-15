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
class AllocaInst;
class FunctionType;
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
	    CodeGenerator &g) = 0;
};

struct IVar : ITyped, IEntity
{
	[[nodiscard]] virtual Ident get_ident() const = 0;
	virtual ast::Expr          *get_init()        = 0;
	virtual llvm::AllocaInst   *get_stack_addr() const = 0;
	virtual void set_stack_addr(llvm::AllocaInst *)    = 0;

	llvm::Value *codegen(CodeGenerator &g);
	llvm::Value *codegen(CodeGenerator &g, llvm::Value *init);
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
	llvm::FunctionType       *get_llvm_type_f(CodeGenerator &g);
	llvm::Type *get_llvm_type(CodeGenerator &g) override;
};

struct IFunc : IFuncType, ITyped
{
public:
	virtual void        set_mangled_name(std::string name);
	virtual std::string get_mangled_name() const = 0;
	virtual IFuncBody  *get_body()               = 0;
	IType              *get_type() override { return this; }
	virtual std::string get_param_name(size_t) const = 0;
	virtual IVar       *get_param(size_t)            = 0;

	llvm::Value *codegen(CodeGenerator &g);

private:
};

struct IFuncBody
{};

} // namespace protolang