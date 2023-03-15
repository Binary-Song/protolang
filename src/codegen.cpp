#include <llvm/ADT/APFloat.h>
#include "ast.h"
#include "code_generator.h"
#include "entity_system.h"
#include "env.h"
#include "exceptions.h"
namespace protolang
{

static llvm::AllocaInst *alloca_for_local_var(
    llvm::Function *func, llvm::Type *type, llvm::StringRef name)
{
	return llvm::IRBuilder<>(&func->getEntryBlock(),
	                         func->getEntryBlock().begin())
	    .CreateAlloca(type, nullptr, name);
}

namespace ast
{
llvm::Value *LiteralExpr::codegen(CodeGenerator &g)
{
	// 生成常数
	if (m_token.type == Token::Type::Fp)
		return llvm::ConstantFP::get(
		    g.context(), llvm::APFloat(this->m_token.fp_data));
	else if (m_token.type == Token::Type::Int)
		return llvm::ConstantInt::get(
		    g.context(),
		    llvm::APInt(32, this->m_token.int_data));
	else
		throw ExceptionNotImplemented{};
}
llvm::Value *IdentExpr::codegen(CodeGenerator &g)
{
	// 变量：引用变量的值，未定义就报错
	// 函数：重载决策后直接得到IFunc了，不用我来生成，
	// 这里只生成变量引用。
	auto var =
	    env()->get<IVar>(ident()); // 从环境中找到对应的var，读取
	g.builder().CreateLoad(var->get_stack_addr()->getAllocatedType(),
	                       var->get_stack_addr(), // 读内存
	                       ident().name); // 给写入的内存取个名称
}

} // namespace ast
llvm::Value *IVar::codegen(CodeGenerator &g)
{
	return codegen(g, this->get_init()->codegen(g));
}
llvm::Value *IVar::codegen(CodeGenerator &g, llvm::Value *init)
{
	// 这个默认是局部变量！！！！！！！！
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!
	assert(this->get_stack_addr() == nullptr);
	// fixme: 目前只能考虑var是局部变量的情形
	auto func = g.builder().GetInsertBlock()->getParent();

	// 在当前函数的入口块申请栈空间
	auto alloca_inst =
	    alloca_for_local_var(func,
	                         this->get_type()->get_llvm_type(g),
	                         this->get_ident().name);

	// 生成初始化表达式的代码
	if (get_init())
	{ // 赋予初始值
		g.builder().CreateStore(init, alloca_inst);
	}
	// 保存栈地址到IVar内部
	this->set_stack_addr(alloca_inst);
	return alloca_inst; // 代表栈空间的位置
}

llvm::Value *IFunc::codegen(CodeGenerator &g)
{
	auto mangled_name = get_mangled_name();
	// get到llvm函数和函数类型
	auto f_type       = this->get_llvm_type_f(g);
	auto f            = llvm::Function::Create(
        f_type,
        llvm::Function::LinkageTypes::InternalLinkage,
        mangled_name,
        g.module());
	// 创建block
	llvm::BasicBlock *bb =
	    llvm::BasicBlock::Create(g.context(), "entry", f);
	g.builder().SetInsertPoint(bb);
	// 遍历参数
	size_t i = 0;
	for (auto &&arg : f->args())
	{
		// LLVM: 设置arg的名字
		arg.setName(this->get_param_name(i));
		// 分配实参的内存，并且将实参赋值给形参
		this->get_param(i)->codegen(g, &arg);
		// 循环
		i++;
	}

	return f;
}

llvm::FunctionType *IFuncType::get_llvm_type_f(CodeGenerator &g)
{
	std::vector<llvm::Type *> ll_types;
	for (size_t i = 0; i < this->get_param_count(); i++)
	{
		auto param_type = this->get_param_type(i);
		ll_types.push_back(param_type->get_llvm_type(g));
	}
	llvm::FunctionType *func_type = llvm::FunctionType::get(
	    get_return_type()->get_llvm_type(g), ll_types, false);
	return func_type;
}

llvm::Type *IFuncType::get_llvm_type(CodeGenerator &g)
{
	return get_llvm_type_f(g);
}

} // namespace protolang