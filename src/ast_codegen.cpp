#include <llvm/ADT/APFloat.h>
#include "ast.h"
#include "code_generator.h"
#include "exceptions.h"
#include "env.h"
namespace protolang
{
namespace ast
{
llvm::Value *LiteralExpr::codegen(CodeGenerator &g) const
{
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
llvm::Value *IdentExpr::codegen(CodeGenerator & ) const
{
	// 变量：引用变量的值，未定义就报错
	// 函数：用到就生成，小心递归
	// todo: 实现具名变量引用
	throw ExceptionNotImplemented{};
}
} // namespace ast
} // namespace protolang