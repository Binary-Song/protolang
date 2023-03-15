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
llvm::Value *IdentExpr::codegen(CodeGenerator &)
{
	// 变量：引用变量的值，未定义就报错
	// 函数：用到就生成原型，不要急着生成函数体
	// todo: 实现具名变量引用
	env()->get(ident());
}

} // namespace ast

llvm::Value *IVar::codegen(CodeGenerator &g)
{
	// fixme: 目前只能考虑var是局部变量的情形
	auto func = g.builder().GetInsertBlock()->getParent();

	// 生成初始化表达式的代码
	auto init = this->get_init()->codegen(g);

	// 在当前函数的入口块申请栈空间
	auto alloca_inst =
	    llvm::IRBuilder<>(&func->getEntryBlock(),
	                      func->getEntryBlock().begin())
	        .CreateAlloca(this->get_type()->get_llvm_type(g),
	                      nullptr,
	                      this->get_ident().name);
	// 赋予初始值
	g.builder().CreateStore(init, alloca_inst);
}

} // namespace protolang