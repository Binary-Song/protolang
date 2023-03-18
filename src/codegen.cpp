#include <llvm/ADT/APFloat.h>
#include "ast.h"
#include "code_generator.h"
#include "entity_system.h"
#include "env.h"
#include "exceptions.h"
namespace protolang
{
static std::vector<IType *> get_arg_types(
    const std::vector<ast::Expr *> &args)
{
	std::vector<IType *> types;
	for (auto &&arg_expr : args)
	{
		auto arg_type = arg_expr->get_type();
		types.push_back(arg_type);
	}
	return types;
}

llvm::Value *ast::Expr::gen_overload_call(
    CodeGenerator                  &g,
    const Ident                    &ident,
    const std::vector<ast::Expr *> &arg_exprs)
{
	auto arg_types = get_arg_types(arg_exprs);
	auto func =
	    this->env()->overload_resolution(ident, arg_types);
	auto func_llvm = func->codegen_func(g);

	size_t i = 0;
	std::vector<llvm::Value *>
	    arg_vals; // 存放经过隐式转换的实参
	for (auto &&arg_expr : arg_exprs)
	{
		auto arg_type   = arg_expr->get_type();
		auto param_type = func->get_param_type(i);

		auto arg_val_raw = arg_expr->codegen_value(g);
		auto arg_val_cast =
		    param_type->cast_implicit(g, arg_val_raw, arg_type);

		arg_vals.push_back(arg_val_cast);
		i++;
	}

	if (func_llvm->arg_size() != arg_vals.size())
	{
		throw ExceptionPanic();
	}

	return g.builder().CreateCall(
	    func_llvm, arg_vals, "calltmp");
}

static llvm::AllocaInst *alloca_for_local_var(
    llvm::Function *func, llvm::Type *type, llvm::StringRef name)
{
	return llvm::IRBuilder<>(&func->getEntryBlock(),
	                         func->getEntryBlock().begin())
	    .CreateAlloca(type, nullptr, name);
}

namespace ast
{
llvm::Value *LiteralExpr::codegen_value(CodeGenerator &g)
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
llvm::Value *IdentExpr::codegen_value(CodeGenerator &g)
{
	// 变量：引用变量的值，未定义就报错
	// 函数：重载决策后直接得到IFunc了，不用我来生成，
	// 这里只生成变量引用。
	auto var =
	    env()->get<IVar>(ident()); // 从环境中找到对应的var，读取
	auto load_inst = g.builder().CreateLoad(
	    var->get_stack_addr()->getAllocatedType(),
	    var->get_stack_addr(), // 读内存
	    ident().name);         // 给写入的内存取个名称
	return load_inst;
}

} // namespace ast
llvm::Value *IVar::codegen_value(CodeGenerator &g)
{
	return codegen_value(g, this->get_init()->codegen_value(g));
}
llvm::Value *IVar::codegen_value(CodeGenerator &g,
                                 llvm::Value   *init)
{
	// 这个默认是局部变量！！！！！！！！
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!
	assert(this->get_stack_addr() == nullptr);

	// 在当前函数的入口块申请栈空间
	auto func = g.builder().GetInsertBlock()->getParent();
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

llvm::Function *IOp::codegen_func(CodeGenerator &g)
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
	this->codegen_param_and_body(g, f);
	return f;
}
void IFunc::codegen_param_and_body(CodeGenerator  &g,
                                   llvm::Function *f)
{
	// 遍历参数
	size_t i = 0;
	// 设置实参的名字 = 形参
	//  形参codegen，需要f
	for (auto &&arg : f->args())
	{
		arg.setName(this->get_param_name(i));
		this->get_param(i)->codegen_value(g, &arg);
		i++;
	}
	get_body()->codegen(g);
}

void ast::CompoundStmt::codegen(CodeGenerator &g)
{
	for (auto &&content : this->m_content)
	{
		content->codegen(g);
	}
}

void ast::ExprStmt::codegen(CodeGenerator &g)
{
	m_expr->codegen(g);
}

llvm::Value *ast::BinaryExpr::codegen_value(CodeGenerator &g)
{
	return gen_overload_call(
	    g, this->op, {left.get(), right.get()});
}

llvm::Value *ast::UnaryExpr::codegen_value(CodeGenerator &g)
{
	return gen_overload_call(g, this->op, {this->operand.get()});
}

llvm::Value *ast::CallExpr::codegen_value(CodeGenerator &g)
{
	// todo: 如果callee不是一个单名，那就根据callee的类型找调用的函数
	if (auto callee =
	        dynamic_cast<IdentExpr *>(this->m_callee.get()))
	{
		std::vector<Expr *> arg_ptrs;
		for (auto &&arg : m_args)
		{
			arg_ptrs.push_back(arg.get());
		}
		return gen_overload_call(g, callee->ident(), arg_ptrs);
	}
	throw ExceptionPanic();
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