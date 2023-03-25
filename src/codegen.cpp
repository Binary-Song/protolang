#include <fmt/xchar.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Verifier.h>
#include "ast.h"
#include "builtin.h"
#include "code_generator.h"
#include "entity_system.h"
#include "env.h"
#include "exceptions.h"
#include "log.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

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
static std::vector<llvm::Value *> cast_args(
    CodeGenerator                  &g,
    IOp                            *func,
    const std::vector<ast::Expr *> &arg_exprs)
{
	std::vector<llvm::Value *> arg_vals;
	size_t                     i = 0;
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
	return arg_vals;
}
static llvm::Value *generate_call(
    CodeGenerator                  &g,
    IOp                            *func,
    const std::vector<ast::Expr *> &arg_exprs)
{
	auto arg_types = get_arg_types(arg_exprs);
	auto args_cast = cast_args(g, func, arg_exprs);
	return func->gen_call(args_cast, g);
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
	else if (m_token.type == Token::Type::Keyword &&
	         m_token.str_data == u8"false")
	{
		llvm::Type *boolType =
		    llvm::Type::getInt1Ty(g.context());
		llvm::Constant *v = llvm::ConstantInt::get(boolType, 0);
		return v;
	}
	else if (m_token.type == Token::Type::Keyword &&
	         m_token.str_data == u8"true")
	{
		llvm::Type *boolType =
		    llvm::Type::getInt1Ty(g.context());
		llvm::Constant *v = llvm::ConstantInt::get(boolType, 1);
		return v;
	}
	else
		throw ExceptionNotImplemented{};
}
llvm::Value *IdentExpr::codegen_value(CodeGenerator &g)
{
	// 变量：引用变量的值，未定义就报错
	// 函数：重载决策后直接得到IFunc了，不用我来生成，
	// 这里只生成变量引用。
	auto var = env()->get_backwards<IVar>(
	    ident()); // 从环境中找到对应的var，读取

	auto load_inst = g.builder().CreateLoad(
	    var->get_stack_addr()->getAllocatedType(),
	    var->get_stack_addr(),  // 读内存
	    ident().name.as_str()); // 给写入的内存取个名称
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
	assert(this->get_stack_addr() == nullptr);

	// 在当前函数的入口块申请栈空间
	auto func = g.builder().GetInsertBlock()->getParent();
	auto alloca_inst =
	    alloca_for_local_var(func,
	                         this->get_type()->get_llvm_type(g),
	                         this->get_ident().name.as_str());

	// 生成初始化表达式的代码
	if (get_init())
	{ // 赋予初始值
		g.builder().CreateStore(init, alloca_inst);
	}
	// 保存栈地址到IVar内部
	this->set_stack_addr(alloca_inst);
	return alloca_inst; // 代表栈空间的位置
}

llvm::Function *IFunc::codegen_prototype(CodeGenerator &g)
{
	auto mangled_name = get_mangled_name();
	auto func_type    = this->get_llvm_func_type(g);
	if (g.module().getFunction(mangled_name.as_str()))
	{
		ErrorFunctionAlreadyExists e;
		e.name = mangled_name;
		throw std::move(e);
	}
	auto func = llvm::Function::Create(
	    func_type,
	    llvm::Function::LinkageTypes::ExternalLinkage,
	    mangled_name.as_str(),
	    g.module());
	return func;
}


llvm::Function *IFunc::codegen_func(CodeGenerator &g)
{
	auto mangled_name = get_mangled_name();

	// 从llvm里查找
	auto func = g.module().getFunction(mangled_name.as_str());
	if (!func)
	{
		// 没有，先登记到llvm 再说
		func = this->codegen_prototype(g);
		assert(func);
	}

	auto block =
	    llvm::BasicBlock::Create(g.context(), "entry", func);
	g.builder().SetInsertPoint(block);

	// 遍历参数
	size_t i = 0;
	// 设置实参的名字 = 形参
	//  形参codegen，需要f
	for (auto &&arg : func->args())
	{
		arg.setName(this->get_param_name(i).as_str());
		this->get_param(i)->codegen_value(g, &arg);
		i++;
	}

	// 生成实现
	get_body()->codegen(g);

	// 检查函数ok不ok
	llvm::EliminateUnreachableBlocks(*func);
	bool error = llvm::verifyFunction(*func);
	if (error)
	{
		ErrorIncompleteBlockInFunc e;
		if (auto function = dynamic_cast<ast::FuncDecl *>(this))
		{
			e.name         = function->get_ident().name;
			e.defined_here = function->range();
			throw std::move(e);
		}
		throw std::move(e);
	}
	return func;
}
llvm::Value *ast::FuncDecl::gen_call(
    std::vector<llvm::Value *> args, CodeGenerator &g)
{
	auto mangled_name = get_mangled_name();
	auto func = g.module().getFunction(mangled_name.as_str());
	if (!func || func->arg_size() != args.size())
	{
		ErrorMissingFunc e;
		e.name = mangled_name;
		throw std::move(e);
	}
	return g.builder().CreateCall(func, args, "calltmp");
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

void ast::ReturnStmt::codegen(CodeGenerator &g)
{
	auto expr_val = get_expr()->codegen_value(g);
	g.builder().CreateRet(expr_val);
}

void ast::ReturnVoidStmt::codegen(CodeGenerator &g)
{
	g.builder().CreateRetVoid();
}

llvm::Value *ast::BinaryExpr::codegen_value(CodeGenerator &g)
{
	return generate_call(
	    g, m_ovlres_cache.get(), {m_left.get(), m_right.get()});
}

llvm::Value *ast::UnaryExpr::codegen_value(CodeGenerator &g)
{
	return generate_call(
	    g, m_ovlres_cache.get(), {this->m_operand.get()});
}

llvm::Value *ast::CallExpr::codegen_value(CodeGenerator &g)
{ // todo:
  // 如果callee不是一个单名，那就根据callee的类型找调用的函数
	if (auto callee =
	        dynamic_cast<IdentExpr *>(this->m_callee.get()))
	{
		std::vector<Expr *> arg_ptrs;
		for (auto &&arg : m_args)
		{
			arg_ptrs.push_back(arg.get());
		}
		return generate_call(g, m_ovlres_cache.get(), arg_ptrs);
	}
	else
	{
		throw ExceptionNotImplemented();
	}
}

llvm::FunctionType *IFuncType::get_llvm_func_type(
    CodeGenerator &g)
{
	std::vector<llvm::Type *> param_types;
	for (size_t i = 0; i < this->get_param_count(); i++)
	{
		auto param_type = this->get_param_type(i);
		param_types.push_back(param_type->get_llvm_type(g));
	}
	llvm::FunctionType *func_type = llvm::FunctionType::get(
	    get_return_type()->get_llvm_type(g), param_types, false);
	return func_type;
}

llvm::Type *IFuncType::get_llvm_type(CodeGenerator &g)
{
	return get_llvm_func_type(g);
}

void ast::IfStmt::codegen(CodeGenerator &g)
{
	auto bool_type     = env()->get_bool();
	auto cond_val_raw  = this->m_condition->codegen_value(g);
	auto cond_val_bool = bool_type->cast_implicit(
	    g, cond_val_raw, m_condition->get_type());
	cond_val_bool->setName("cond");

	auto func = g.builder().GetInsertBlock()->getParent();

	auto then_blk =
	    llvm::BasicBlock::Create(g.context(), "then");
	auto merge_blk =
	    llvm::BasicBlock::Create(g.context(), "merge");

	if (m_else.has_value())
	{
		auto else_blk =
		    llvm::BasicBlock::Create(g.context(), "else");
		g.builder().CreateCondBr(
		    cond_val_bool, then_blk, else_blk);
		this->generate_branch(
		    g, func, then_blk, m_then, merge_blk);
		this->generate_branch(
		    g, func, else_blk, m_else.value(), merge_blk);
	}
	else
	{
		g.builder().CreateCondBr(
		    cond_val_bool, then_blk, merge_blk);
		this->generate_branch(
		    g, func, then_blk, m_then, merge_blk);
	}

	func->insert(func->end(), merge_blk);
	g.builder().SetInsertPoint(
	    merge_blk); // 确保结束时插入点是merge块，见上
}
void ast::IfStmt::generate_branch(
    CodeGenerator                 &g,
    llvm::Function                *func,
    llvm::BasicBlock              *branch_blk,
    std::unique_ptr<CompoundStmt> &branch_ast,
    llvm::BasicBlock              *merge_blk)
{
	func->insert(func->end(), branch_blk);
	g.builder().SetInsertPoint(branch_blk);
	branch_ast->codegen(g);
	// 假设then里面还套if，那现在的插入点是then内部if的末尾merge块
	// 如果以后还要往then插，就直接往then分支的末尾插了
	branch_blk      = g.builder().GetInsertBlock();
	auto terminator = branch_blk->getTerminator();
	if (!terminator || !llvm::isa<llvm::ReturnInst>(terminator))
		g.builder().CreateBr(merge_blk); // 无条件跳转
}

} // namespace protolang