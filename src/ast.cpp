#include <fmt/xchar.h>
#include <utility>
#include "ast.h"
#include "encoding.h"
#include "entity_system.h"
#include "env.h"
#include "log.h"
#include "logger.h"

namespace protolang::ast
{

// === IdentTypeExpr ===
IdentTypeExpr::IdentTypeExpr(Env *env, Ident ident)
    : m_ident(std::move(ident))
    , m_env(env)
    , m_type_cache(
          [this]()
          {
	          return this->recompute_type();
          })
{}

IType *IdentTypeExpr::get_type()
{
	return m_type_cache.get();
}
IType *IdentTypeExpr::recompute_type()
{
	return this->env()->get<IType>(ident());
}

void IdentTypeExpr::validate_types()
{
	[[maybe_unused]] auto _ = get_type();
}
StringU8 IdentTypeExpr::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"IdentTypeExpr","ident":{}}})",
	    m_ident.dump_json());
}

// === BinaryExpr ===
BinaryExpr::BinaryExpr(uptr<Expr> left,
                       Ident      op,
                       uptr<Expr> right)
    : m_left(std::move(left))
    , m_op(std::move(op))
    , m_right(std::move(right))
    , m_type_cache(
          [this]()
          {
	          return m_ovlres_cache.get()->get_return_type();
          })
    , m_ovlres_cache(
          [this]()
          {
	          auto lhs_type = this->m_left->get_type();
	          auto rhs_type = this->m_right->get_type();
	          return this->env()->overload_resolution(
	              m_op, {lhs_type, rhs_type});
          })
{}
IType *BinaryExpr::get_type()
{
	return m_type_cache.get();
}

// === UnaryExpr ===
UnaryExpr::UnaryExpr(bool prefix, uptr<Expr> operand, Ident op)
    : m_prefix(prefix)
    , m_operand(std::move(operand))
    , m_op(std::move(op))
    , m_type_cache(
          [this]()
          {
	          return m_ovlres_cache.get()->get_return_type();
          })
    , m_ovlres_cache(
          [this]()
          {
	          auto operand_type = m_operand->get_type();
	          return env()->overload_resolution(m_op,
	                                            {operand_type});
          })
{}
IType *UnaryExpr::get_type()
{
	return m_type_cache.get();
}
StringU8 UnaryExpr::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"UnaryExpr","op":{},"oprd":{}}})",
	    m_op.dump_json(),
	    m_operand->dump_json());
}

// === CallExpr ===
CallExpr::CallExpr(const SrcRange         &src_rng,
                   uptr<Expr>              callee,
                   std::vector<uptr<Expr>> args)
    : m_callee(std::move(callee))
    , m_args(std::move(args))
    , m_src_rng(src_rng)
    , m_type_cache(
          [this]()
          {
	          return this->recompute_type();
          })
    , m_ovlres_cache(
          [this]() -> IOp *
          {
	          if (auto ident_expr =
	                  dynamic_cast<IdentExpr *>(m_callee.get()))
	          {
		          IOp *func = env()->overload_resolution(
		              ident_expr->ident(), get_arg_types());
		          return func;
	          }
	          return nullptr;
          })
{}
IType *CallExpr::get_arg_type(size_t index)
{
	return m_args[index]->get_type();
}
IType *CallExpr::get_type()
{
	return m_type_cache.get();
}
IType *CallExpr::recompute_type()
{
	// non-member call: func(1,2,3)
	// member call:
	// - a.func(1,2,3)
	// - (((a.b).c).func)(1,2,3)
	// 如何判断是不是调用成员函数？
	// 1. 看callee是不是MemberAccessExpr
	// 2.
	// 如果是，看member是不是成员函数类型（而不是成员函数指针等callable对象）
	// 3. 如果是，那没跑了，直接按成员函数法调用。
	// todo: 实现成员函数
	// 另外，如果callee是个名字，则执行overload resolution，
	// 如果callee是个表达式，则用不着执行overload resolution.

	// 目前没有成员函数，这是调用自由函数的情况（进行重载决策）
	if (auto ident_expr =
	        dynamic_cast<IdentExpr *>(m_callee.get()))
	{
		IOp *func        = m_ovlres_cache.get();
		auto return_type = func->get_return_type();
		auto func_type   = func->get_type();
		ident_expr->set_type(func_type);
		return return_type;
	}
	// 否则，如果是函数指针等，不用重载决策直接调用
	else if (auto func_type =
	             dynamic_cast<IOp *>(m_callee->get_type()))
	{
		// 检查参数类型
		try
		{
			env()->check_args(
			    func_type, get_arg_types(), true, false);
		}
		catch (ErrorCallTypeMismatch &e)
		{
			e.call = this->range();
			throw std::move(e);
		}
		catch (ErrorCallArgCountMismatch &e)
		{
			e.call = this->range();
			throw std::move(e);
		}
		return func_type->get_return_type();
	}
	else
	{
		ErrorNotCallable e;
		e.callee = m_callee->range();
		e.type   = m_callee->get_type()->get_type_name();
		throw std::move(e);
	}
}
StringU8 CallExpr::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"CallExpr","callee":{},"args":{}}})",
	    m_callee->dump_json(),
	    dump_json_for_vector_of_ptr(m_args));
}
// === BracketExpr ===
StringU8 BracketExpr::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"BracketExpr","callee":{},"args":{}}})",
	    get_callee()->dump_json(),
	    dump_json_for_vector_of_ptr(m_args));
}
// === MemberAccessExpr ===
MemberAccessExpr::MemberAccessExpr(uptr<Expr> left, Ident member)
    : m_left(std::move(left))
    , m_member(std::move(member))
    , m_type_cache(
          [this]()
          {
	          return recompute_type();
          })
{}
IType *MemberAccessExpr::get_type()
{
	return m_type_cache.get();
}
IType *MemberAccessExpr::recompute_type()
{
	auto member_entity =
	    m_left->get_type()->get_member(m_member);
	if (member_entity)
	{
		if (auto member_func =
		        dynamic_cast<IOp *>(member_entity))
		{
			// todo : member func 的 type 应该不一样！但我建议
			// 不要在这里改，在 IFunc 的实现类改！
			return member_func->get_type();
		}
		else if (auto member_var =
		             dynamic_cast<IVar *>(member_entity))
		{
			return member_var->get_type();
		}
	}
	ErrorMemberNotFound e;
	e.type      = m_left->get_type()->get_type_name();
	e.member    = m_member.name;
	e.used_here = m_member.range;
	throw std::move(e);
}
llvm::Value *MemberAccessExpr::codegen_value(
    [[maybe_unused]] CodeGenerator &g)
{
	throw ExceptionNotImplemented{};
}
StringU8 MemberAccessExpr::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"MemberAccessExpr","left":{},"member":{}}})",
	    m_left->dump_json(),
	    m_member.dump_json());
}

// === LiteralExpr ===
IType *LiteralExpr::get_type()
{
	return m_type_cache.get();
}

IType *LiteralExpr::recompute_type()
{
	if (m_token.type == Token::Type::Int)
	{
		return root_env()->get<IType>(
		    Ident(u8"int", m_token.range()));
	}
	else if (m_token.type == Token::Type::Fp)
	{
		return root_env()->get<IType>(
		    Ident(u8"double", m_token.range()));
	}
	assert(false); // 没有这种literal
	return nullptr;
}
StringU8 LiteralExpr::dump_json()
{
	return fmt::format(u8"\"{}/{}/{}\"",
	                   m_token.str_data,
	                   m_token.int_data,
	                   m_token.fp_data);
}

// === IdentExpr ===
IType *IdentExpr::get_type()
{
	return m_type_cache.get();
}
IType *IdentExpr::recompute_type()
{
	auto entity = env()->get(ident());
	if (auto var = dynamic_cast<IVar *>(entity))
	{
		// 解决var a = 1 + a; 解析a类型时无限递归报错
		check_forward_ref<true>(this->ident(), var);
	}
	if (dynamic_cast<OverloadSet *>(entity))
	{
		// 如果标识符是重载集合，则需要上层表达式来帮忙决定type
		ErrorNameInThisContextIsAmbiguous e;
		e.name = this->ident();
		throw std::move(e);
	}
	if (auto typed = dynamic_cast<ITyped *>(entity))
	{
		return typed->get_type();
	}
	ErrorNameInThisContextIsAmbiguous e;
	e.name = this->ident();
	throw std::move(e);
}

void IdentExpr::set_type(IType *t)
{
	m_type_cache.set(t);
}
StringU8 IdentExpr::dump_json()
{
	return fmt::format(u8"\"{}\"", m_ident.name);
}

// === Block ===

void VarDecl::validate_types()
{
	assert(m_init || m_type);

	auto init_type = m_init->get_type();
	auto var_type  = this->get_type();
	if (!var_type->can_accept(init_type))
	{
		ErrorVarDeclInitExprTypeMismatch e;
		e.init_type    = init_type->get_type_name();
		e.var_type     = var_type->get_type_name();
		e.init_range   = m_init->range();
		e.var_ty_range = m_type->range();
		throw std::move(e);
	}
}
StringU8 VarDecl::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"VarDecl","ident":{},"type":{},"init":{}}})",
	    m_ident.dump_json(),
	    get_type()->dump_json(),
	    m_init->dump_json());
}
StringU8 ParamDecl::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"ParamDecl","ident":{},"type":{} }})",
	    m_ident.dump_json(),
	    m_type->dump_json());
}
FuncDecl::FuncDecl(Env                         *env,
                   SrcRange                     range,
                   Ident                        ident,
                   std::vector<uptr<ParamDecl>> params,
                   uptr<TypeExpr>               return_type,
                   uptr<CompoundStmt>           body)
    : m_env(env)
    , m_range(range)
    , m_ident(std::move(ident))
    , m_params(std::move(params))
    , m_return_type(std::move(return_type))
    , m_body(std::move(body))
{}

StructDecl::StructDecl(Env             *env,
                       const SrcRange  &range,
                       Ident            ident,
                       uptr<StructBody> body)
    : m_env(env)
    , m_range(range)
    , m_ident(std::move(ident))
    , m_body(std::move(body))
{}

bool StructDecl::can_accept(IType *other)
{
	return this->equal(other);
}

bool StructDecl::equal(IType *other)
{
	return this == dynamic_cast<const StructDecl *>(other);
}

StringU8 StructDecl::get_type_name()
{
	return m_ident.name;
}

Env *Ast::root_env() const
{
	return env()->get_root();
}

Program::Program(std::vector<uptr<Decl>> decls, Logger &logger)
    : m_decls(std::move(decls))
    , m_root_env(Env::create_root(logger))
    , logger(logger)
{}
void Program::validate_types(bool &success)
{
	try
	{
		for (auto &&decl : m_decls)
		{
			decl->validate_types();
		}
	}
	catch (Error &e)
	{
		e.print(logger);
		success = false;
	}
	success = true;
}
void Program::validate_types()
{
	assert(false);
}
void Program::codegen(CodeGenerator &g, bool &success)
{
	try
	{
		// 先 生成函数的prototype
		for (auto &&d : m_decls)
		{
			if (auto func_decl =
			        dynamic_cast<FuncDecl *>(d.get()))
			{
				func_decl->codegen_prototype(g);
			}
		}

		for (auto &&d : m_decls)
		{
			d->codegen(g);
		}

		success = true;
	}
	catch (Error &e)
	{
		e.print(logger);
		success = false;
	}
}
void Program::codegen(CodeGenerator &)
{
	assert(false);
}
StringU8 Program::dump_json()
{
	return fmt::format(
		u8R"({{"obj":"Program","decls":[{}]}})",
		dump_json_for_vector_of_ptr(m_decls));
}

// 表达式默认的语义检查方法是计算一次类型

void Expr::validate_types()
{
	[[maybe_unused]] auto _ = get_type();
}

} // namespace protolang::ast