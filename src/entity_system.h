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
class Function;
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

// 有名之物，曰实体
// 声明会引入实体，并存放到作用域内。
// 有名，但不要求保存它。因此不提供get_name虚函数。
// 表达式不算实体。因为没有名字。
struct IEntity : virtual IJsonDumper
{
	~IEntity() override = default;
};

// 有类型之物，函数、变量、表达式都是有类型的。
// 类型自己不算有类型。
struct ITyped : virtual IJsonDumper
{
	~ITyped() override        = default;
	virtual IType *get_type() = 0;
};

struct TypeCache
{
private:
	IType *m_type_cache = nullptr;

public:
	IType *lazy_get_type()
	{
		if (m_type_cache)
			return m_type_cache;
		m_type_cache = recompute_type();
		return m_type_cache;
	}
	virtual IType *recompute_type() = 0;

protected:
	void set_type_cache(IType *t) { m_type_cache = t; }
};

// 类型
struct IType : IEntity
{
	~IType() override                       = default;
	virtual bool        can_accept(IType *) = 0;
	virtual bool        equal(IType *)      = 0;
	virtual std::string get_type_name()     = 0;
	virtual IEntity    *get_member(const Ident &)
	{
		return nullptr;
	}
	virtual llvm::Type *get_llvm_type(CodeGenerator &g) = 0;

	// 将类型为type的val转换到本类型，将检查是不是equal以及can_accept。
	llvm::Value *cast_implicit(CodeGenerator &g,
	                           llvm::Value   *val,
	                           IType         *type);

	// 将类型为type的val转换到本类型，将检查是不是equal。
	llvm::Value *cast_explicit(CodeGenerator &g,
	                           llvm::Value   *val,
	                           IType         *type);

private:
	// 本函数不负责检查src是不是本类型。能cast的尽量cast。
	virtual llvm::Value *cast_inst_no_check(CodeGenerator &g,
	                                        llvm::Value   *val,
	                                        IType *type) = 0;
};

struct ICodeGen
{
	virtual ~ICodeGen()                    = default;
	virtual void codegen(CodeGenerator &g) = 0;
};

struct IVar : ITyped, IEntity
{
	virtual Ident             get_ident() const      = 0;
	virtual ast::Expr        *get_init()             = 0;
	virtual llvm::AllocaInst *get_stack_addr() const = 0;
	virtual void set_stack_addr(llvm::AllocaInst *)  = 0;

	llvm::Value *codegen_value(CodeGenerator &g);
	llvm::Value *codegen_value(CodeGenerator &g,
	                           llvm::Value   *init);
};

struct IFuncType : IType
{
	virtual IType *get_return_type()       = 0;
	virtual size_t get_param_count() const = 0;
	virtual IType *get_param_type(size_t)  = 0;

	// === 实现 IType  ===
	bool can_accept(IType *other) override
	{
		return this->equal(other);
	}
	bool equal(IType *other) override
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
	std::string         get_type_name() override;
	llvm::FunctionType *get_llvm_func_type(CodeGenerator &g);
	llvm::Type         *get_llvm_type(CodeGenerator &g) override;

private:
	llvm::Value *cast_inst_no_check(
	    [[maybe_unused]] CodeGenerator &g,
	    [[maybe_unused]] llvm::Value   *val,
	    [[maybe_unused]] IType         *type) override
	{
		return nullptr;
	}
};

/// 运算符或函数。在此抽象级别无法获取IVar类型的参数（IVar占用栈空间），
/// 但可以获取参数类型。
/// 内置运算符或函数可以实现本接口。
struct IOp : virtual ITyped, IFuncType
{
	virtual std::string get_mangled_name() const           = 0;
	virtual void        set_mangled_name(std::string name) = 0;
	IType              *get_type() override { return this; }

	llvm::Function *get_func(CodeGenerator &g);
	llvm::Function *codegen_func(CodeGenerator &g);
	llvm::Function *codegen_prototype(CodeGenerator &g);

private:
	/// 为参数和函数体生成代码。
	/// 实现本函数时，实参用f->args()获取，形参自己实现。
	virtual void codegen_param_and_body(CodeGenerator  &g,
	                                    llvm::Function *f) = 0;
};

/// 用户定义的函数，有参数（占用栈空间）和函数体。
struct IFunc : IOp
/* 继承是能力的拓展，不是所谓的is-a */
{
public:
	virtual ICodeGen   *get_body()                   = 0;
	virtual std::string get_param_name(size_t) const = 0;
	virtual IVar       *get_param(size_t)            = 0;

private:
	virtual void codegen_param_and_body(
	    CodeGenerator &g, llvm::Function *f) override;
};

} // namespace protolang