#include <llvm/ADT/APFloat.h>
#include "ast.h"
#include "code_generator.h"
#include "entity_system.h"
#include "env.h"
#include "exceptions.h"
namespace protolang
{
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
	    env()->get<IVar>(ident()); // 从环境中找到对应的var
	g.builder().CreateLoad(var->get_value()->getAllocatedType(),
	                       var->get_value(),
	                       ident().name);
}

} // namespace ast

llvm::Value *IVar::codegen(CodeGenerator &g)
{
	assert(this->get_value() == nullptr);
	// fixme: 目前只能考虑var是局部变量的情形
	auto func = g.builder().GetInsertBlock()->getParent();

	// 在当前函数的入口块申请栈空间
	auto alloca_inst =
	    llvm::IRBuilder<>(&func->getEntryBlock(),
	                      func->getEntryBlock().begin())
	        .CreateAlloca(this->get_type()->get_llvm_type(g),
	                      nullptr,
	                      this->get_ident().name);

	// 生成初始化表达式的代码

	auto init = this->get_init();
	if (init)
	{
		// 赋予初始值
		g.builder().CreateStore(init->codegen(g), alloca_inst);
	}
	this->set_value(alloca_inst);
	return alloca_inst; // 代表栈空间的位置
}

llvm::Value *IFunc::codegen(CodeGenerator &g)
{

}

llvm::Type  *IFuncType::get_llvm_type(CodeGenerator &g)
{
	std::vector<llvm::Type *> ll_types;
	for (size_t i = 0; i < this->get_param_count(); i++)
	{
		auto param_type = this->get_param_type(i);
		ll_types.push_back(param_type->get_llvm_type(g));
	}

	llvm::FunctionType *ft = llvm::FunctionType::get(
	    get_return_type()->get_llvm_type(g), ll_types, false);

	return ft;
}
} // namespace protolang