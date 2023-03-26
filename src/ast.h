#pragma once
#include <cassert>
#include <fmt/xchar.h>
#include <functional>
#include <llvm/IR/BasicBlock.h>
#include <optional>
#include <utility>
#include <vector>
#include "cache.h"
#include "encoding.h"
#include "entity_system.h"
#include "ident.h"
#include "token.h"
#include "util.h"
namespace llvm
{
class Value;
}

namespace protolang
{
struct IType;
class Env;
class Logger;
struct CodeGenerator;
namespace ast
{

struct IBlockContent
{
	virtual ~IBlockContent() = default;
};

struct ICompoundStmtContent : IBlockContent,
                              virtual IJsonDumper,
                              virtual ICodeGen
{
	virtual void validate_types(IType *return_type) = 0;
};

struct IStructContent : IBlockContent,
                        virtual IJsonDumper,
                        virtual ICodeGen
{
	virtual void validate_types() = 0;
};

////////////////////////

struct Ast : virtual IJsonDumper, virtual ICodeGen
{
	// 函数
public:
	Env *root_env() const;

	// 虚函数
public:
	~Ast() override                = default;
	virtual SrcRange range() const = 0;
	virtual Env     *env() const   = 0;
};

// 类型表达式，这是类型的引用，并不是真正的类型声明，抽象类
struct TypeExpr : Ast
{
	virtual IType *get_type() = 0;

	void validate_types() { get_type(); }
};

// 类型标识符
struct IdentTypeExpr : TypeExpr
{
	// 数据
private:
	Ident        m_ident;
	Env         *m_env;
	Cache<IType> m_type_cache;

	// 函数
public:
	explicit IdentTypeExpr(Env *env, Ident ident);
	Ident    ident() const { return m_ident; }
	// 实现基类成员
	StringU8 dump_json() override;
	SrcRange range() const override { return m_ident.range; }
	Env     *env() const override { return m_env; }
	IType   *get_type() override;
	void     codegen(CodeGenerator &) override {}

private:
	IType *recompute_type();
};

// 表达式，抽象类
struct Expr : Ast, ITyped
{
	// 类
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	};

	virtual ValueCat get_cat() const
	{
		return ValueCat::Pending;
	}
	// 表达式默认的语义检查方法是计算一次类型
	void validate_types() { get_type(); }
	void codegen(CodeGenerator &g) override { codegen_value(g); }
	virtual llvm::Value *codegen_value(CodeGenerator &g) = 0;
};

// 二元运算表达式
struct BinaryExpr : Expr
{
	// 数据
protected:
	uptr<Expr>   m_left;
	uptr<Expr>   m_right;
	Ident        m_op;
	Cache<IType> m_type_cache;
	Cache<IOp>   m_ovlres_cache;
	// 函数
public:
	BinaryExpr(uptr<Expr> left, Ident op, uptr<Expr> right);
	Expr    *get_left() { return m_left.get(); }
	Expr    *get_right() { return m_right.get(); }
	Ident    get_op() const { return m_op; }
	SrcRange range() const override
	{
		return m_left->range() + m_right->range();
	}
	Env *env() const override
	{
		assert(m_left->env() == m_right->env());
		return m_left->env();
	}
	IType   *get_type() override;
	StringU8 dump_json() override
	{
		return fmt::format(
		    u8R"({{"obj":"BinaryExpr","op":{},"lhs":{},"rhs":{}}})",
		    m_op.dump_json(),
		    m_left->dump_json(),
		    m_right->dump_json());
	}
	llvm::Value *codegen_value(CodeGenerator &g) override;
};

// 一元运算
struct UnaryExpr : public Expr
{
	// 数据
private:
	bool         m_prefix;
	uptr<Expr>   m_operand;
	Ident        m_op;
	Cache<IType> m_type_cache;
	Cache<IOp>   m_ovlres_cache;

	// 函数
public:
	UnaryExpr(bool prefix, uptr<Expr> operand, Ident op);

	bool     is_prefix() const { return m_prefix; }
	Expr    *get_operand() { return m_operand.get(); }
	Ident    get_op() const { return m_op; }
	StringU8 dump_json() override;
	SrcRange range() const override
	{
		return m_op.range + m_operand->range();
	}
	Env   *env() const override { return m_operand->env(); }
	IType *get_type() override;
	llvm::Value *codegen_value(CodeGenerator &g) override;
};

struct AsExpr : Expr
{
	uptr<TypeExpr> m_type;
	uptr<Expr>     m_operand;

	AsExpr(std::unique_ptr<TypeExpr> mType,
	       std::unique_ptr<Expr>     mOperand)
	    : m_type(std::move(mType))
	    , m_operand(std::move(mOperand))
	{}

	StringU8 dump_json() override { return "AsExpr"; }
	SrcRange range() const override
	{
		return m_type->range() + m_operand->range();
	}
	Env   *env() const override { return m_operand->env(); }
	IType *get_type() override;
	llvm::Value *codegen_value(CodeGenerator &g) override;
};

struct CallExpr : Expr
{
	// 数据
protected:
	uptr<Expr>              m_callee;
	std::vector<uptr<Expr>> m_args;
	SrcRange                m_src_rng;
	Cache<IType>            m_type_cache;
	Cache<IOp, true>        m_ovlres_cache;

public:
	CallExpr(const SrcRange         &src_rng,
	         uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args);

	Expr  *get_callee() const { return m_callee.get(); }
	size_t get_arg_count() const { return m_args.size(); }
	Expr  *get_arg(size_t index) const
	{
		return m_args[index].get();
	}
	SrcRange             get_range() const { return m_src_rng; }
	IType               *get_arg_type(size_t index);
	std::vector<IType *> get_arg_types()
	{
		std::vector<IType *> types;
		for (size_t i = 0; i < get_arg_count(); i++)
		{
			types.push_back(get_arg_type(i));
		}
		return types;
	}
	StringU8     dump_json() override;
	SrcRange     range() const override { return m_src_rng; }
	Env         *env() const override { return m_callee->env(); }
	IType       *get_type() override;
	llvm::Value *codegen_value(CodeGenerator &g) override;

private:
	IType *recompute_type();
};

struct BracketExpr : CallExpr
{
public:
	BracketExpr(const SrcRange         &src_rng,
	            uptr<Expr>              callee,
	            std::vector<uptr<Expr>> args)
	    : CallExpr(src_rng, std::move(callee), std::move(args))
	{}
	StringU8 dump_json() override;
};
struct IdentExpr;
struct MemberAccessExpr : Expr
{
	// 数据
protected:
	uptr<Expr>   m_left;
	Ident        m_member;
	Cache<IType> m_type_cache;
	// 函数
public:
	MemberAccessExpr(uptr<Expr> left, Ident member);

	Expr    *get_left() { return m_left.get(); }
	Ident    get_member() { return m_member; }
	StringU8 dump_json() override;
	SrcRange range() const override
	{
		return m_left->range() + m_member.range;
	}
	Env         *env() const override { return m_left->env(); }
	IType       *get_type() override;
	llvm::Value *codegen_value(CodeGenerator &g) override;

private:
	IType *recompute_type();
};

struct LiteralExpr : Expr
{
public:
	Token        m_token;
	Env         *m_env;
	Cache<IType> m_type_cache;

public:
	explicit LiteralExpr(Env *env, Token token)
	    : m_env(env)
	    , m_token(std::move(token))
	    , m_type_cache(
	          [this]()
	          {
		          return recompute_type();
	          })
	{}

	StringU8 dump_json() override;
	SrcRange range() const override { return m_token.range(); }
	Env     *env() const override { return m_env; }
	IType   *get_type() override;
	Token    get_token() const { return m_token; }
	llvm::Value *codegen_value(CodeGenerator &g) override;

private:
	IType *recompute_type();
};

struct IdentExpr : Expr
{
private:
	Ident        m_ident;
	Env         *m_env;
	Cache<IType> m_type_cache;

public:
	explicit IdentExpr(Env *env, Ident ident)
	    : m_env(env)
	    , m_ident(std::move(ident))
	    , m_type_cache(
	          [this]()
	          {
		          return recompute_type();
	          })
	{}

	Ident        ident() const { return m_ident; }
	StringU8     dump_json() override;
	SrcRange     range() const override { return m_ident.range; }
	Env         *env() const override { return m_env; }
	IType       *get_type() override;
	void         set_type(IType *type);
	llvm::Value *codegen_value(CodeGenerator &g) override;

private:
	IType *recompute_type();
};

struct Decl : virtual Ast
{
	virtual void validate_types() = 0;
};

struct VarDecl : Decl, IVar, ICompoundStmtContent
{
private:
	Ident             m_ident;
	uptr<TypeExpr>    m_type;
	uptr<Expr>        m_init;
	llvm::AllocaInst *m_value = nullptr;

public:
	VarDecl(Ident ident, uptr<TypeExpr> type, uptr<Expr> init)
	    : m_ident(std::move(ident))
	    , m_type(std::move(type))
	    , m_init(std::move(init))
	{}
	Ident  get_ident() const override { return m_ident; }
	IType *get_type() override
	{
		return m_type ? m_type->get_type() : m_init->get_type();
	}
	Expr    *get_init() override { return m_init.get(); }
	StringU8 dump_json() override;
	SrcRange range() const override
	{
		return m_ident.range + m_init->range();
	}
	Env *env() const override
	{
		return m_type ? m_type->env() : m_init->env();
	}
	void validate_types() override;
	void validate_types(IType *) override
	{
		return validate_types();
	}
	llvm::AllocaInst *get_stack_addr() const override
	{
		return m_value;
	}
	void set_stack_addr(llvm::AllocaInst *value) override
	{
		m_value = value;
	}
	void codegen(CodeGenerator &g) override
	{
		this->codegen_value(g);
	}
};

struct ParamDecl : Decl, IVar
{
private:
	Env           *m_env; // env在函数内部
	Ident          m_ident;
	uptr<TypeExpr> m_type; // 注意，它的env在函数外部
	llvm::AllocaInst *m_value = nullptr;

public:
	ParamDecl(Env *env, Ident ident, uptr<TypeExpr> type)
	    : m_env(env)
	    , m_ident(std::move(ident))
	    , m_type(std::move(type))
	{}

	Ident    get_ident() const override { return m_ident; }
	Expr    *get_init() override { return nullptr; }
	IType   *get_type() override { return m_type->get_type(); }
	StringU8 dump_json() override;
	SrcRange range() const override
	{
		return m_ident.range + m_type->range();
	}
	Env *env() const override { return m_env; }
	void validate_types() override { this->m_type->get_type(); }
	llvm::AllocaInst *get_stack_addr() const override
	{
		return m_value;
	}
	void set_stack_addr(llvm::AllocaInst *value) override
	{
		m_value = value;
	}
	void codegen(CodeGenerator &g) override
	{
		this->codegen_value(g);
	}
};

struct Stmt : virtual Ast, ICompoundStmtContent
{
	/// 如果语句返回，则返回值类型由return_type给出
	virtual void validate_types(IType *return_type) = 0;
};

struct ExprStmt : Stmt
{
private:
	SrcRange   m_range;
	uptr<Expr> m_expr;

public:
	explicit ExprStmt(const SrcRange &range, uptr<Expr> expr)
	    : m_expr(std::move(expr))
	    , m_range(range)
	{}
	Expr    *get_expr() { return m_expr.get(); }
	StringU8 dump_json() override;
	SrcRange range() const override { return m_range; }
	Env     *env() const override { return m_expr->env(); }
	void validate_types(IType *) override { m_expr->get_type(); }
	void codegen(CodeGenerator &g) override;
};

struct IScope : virtual Ast
{
	virtual void set_outer_env(Env *env) = 0;
	virtual Env *get_outer_env() const   = 0;
	virtual void set_inner_env(Env *env) = 0;
	virtual Env *get_inner_env() const   = 0;

	template <std::derived_from<IScope> ScopeType>
	static uptr<ScopeType> nest(Env *outer_env)
	{
		auto ptr = make_uptr(new ScopeType());
		ptr->set_outer_env(outer_env);
		auto inner_env = create_env(
		    ptr->get_outer_env(), ptr->get_outer_env()->logger);
		ptr->set_inner_env(inner_env);
		return ptr;
	}
};

template <std::derived_from<IBlockContent> TContent>
struct IBlock : IScope
{
	virtual void      set_range(const SrcRange &range) = 0;
	virtual void      add_content(uptr<TContent>)      = 0;
	virtual TContent *get_content(size_t i)            = 0;
	virtual size_t    get_content_size() const         = 0;
};

struct CompoundStmt : Stmt, IBlock<ICompoundStmtContent>
{
private:
	SrcRange m_range;
	Env     *m_inner_env = nullptr;
	Env     *m_outer_env = nullptr;

	std::vector<uptr<ICompoundStmtContent>> m_content;

public:
	CompoundStmt() = default;

	SrcRange range() const override { return m_range; }

	Env *env() const override { return get_outer_env(); }
	Env *get_outer_env() const override { return m_outer_env; }
	Env *get_inner_env() const override { return m_inner_env; }
	void set_outer_env(Env *env) override { m_outer_env = env; }
	void set_inner_env(Env *env) override { m_inner_env = env; }

	void set_range(const SrcRange &range) override
	{
		m_range = range;
	}
	void add_content(uptr<ICompoundStmtContent> content) override
	{
		m_content.push_back(std::move(content));
	}
	ICompoundStmtContent *get_content(size_t i) override
	{
		return m_content[i].get();
	}
	size_t get_content_size() const override
	{
		return m_content.size();
	}
	StringU8 dump_json() override
	{
		return dump_json_for_vector_of_ptr(m_content);
	}
	void validate_types(IType *return_type) override
	{
		for (auto &&elem : m_content)
		{
			elem->validate_types(return_type);
		}
	}
	void codegen(CodeGenerator &g) override;
};

struct ReturnStmt : Stmt
{
private:
	SrcRange   m_range;
	uptr<Expr> m_expr;

public:
public:
	explicit ReturnStmt(const SrcRange &range, uptr<Expr> expr)
	    : m_expr(std::move(expr))
	    , m_range(range)
	{}
	Expr    *get_expr() { return m_expr.get(); }
	StringU8 dump_json() override;
	SrcRange range() const override { return m_range; }
	Env     *env() const override { return m_expr->env(); }
	void     codegen(CodeGenerator &g) override;
	void     validate_types(IType *return_type) override;
};
struct ReturnVoidStmt : Stmt
{
private:
	Env     *m_env;
	SrcRange m_range;

public:
	explicit ReturnVoidStmt(const SrcRange &range, Env *env)
	    : m_env(env)
	    , m_range(range)
	{}
	StringU8 dump_json() override
	{
		return u8R"({{"obj":"ReturnVoidStmt"}})";
	}
	SrcRange range() const override { return m_range; }
	Env     *env() const override { return m_env; }
	void     codegen(CodeGenerator &g) override;
	void     validate_types(IType *return_type) override;
};

struct IfStmt : Stmt
{
protected:
	Env                              *m_env;
	Token                             m_if_token;
	uptr<Expr>                        m_condition;
	uptr<CompoundStmt>                m_then;
	std::optional<uptr<CompoundStmt>> m_else;

public:
	IfStmt(Env               *env,
	       Token              if_token,
	       uptr<Expr>         cond,
	       uptr<CompoundStmt> then);

	void set_else_clause(uptr<CompoundStmt> else_clause)
	{
		m_else = std::move(else_clause);
	}

	SrcRange range() const override
	{
		if (m_else.has_value())
		{
			return m_if_token.range() + m_else.value()->range();
		}
		return m_if_token.range() + m_then->range();
	}
	Env     *env() const override { return m_env; }
	void     validate_types(IType *return_type) override;
	void     codegen(CodeGenerator &g) override;
	StringU8 dump_json() override;

private:
	void generate_branch(
	    CodeGenerator                 &g,
	    llvm::Function                *func,
	    llvm::BasicBlock              *branch_blk,
	    std::unique_ptr<CompoundStmt> &branch_ast,
	    llvm::BasicBlock              *merge_blk);
};

struct FuncDecl : Decl, IFunc
{
private:
	Env                         *m_env = nullptr;
	SrcRange                     m_range;
	Ident                        m_ident;
	std::vector<uptr<ParamDecl>> m_params;
	uptr<TypeExpr>               m_return_type;
	uptr<CompoundStmt>           m_body;
	StringU8                     m_mangled_name;

public:
	FuncDecl() = default;
	FuncDecl(Env                         *env,
	         SrcRange                     range,
	         Ident                        ident,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	StringU8 dump_json() override;
	Ident    get_ident() const { return m_ident; }
	IType   *get_type() override { return this; }
	SrcRange range() const override { return m_range; }
	Env     *env() const override { return m_env; }

	IType *get_return_type() override
	{
		return m_return_type->get_type();
	}
	size_t get_param_count() const override
	{
		return m_params.size();
	}
	IType *get_param_type(size_t i) override
	{
		return m_params[i]->get_type();
	}
	ICodeGen *get_body() override { return m_body.get(); }
	void      validate_types() override;
	StringU8  get_mangled_name() const override
	{
		return m_mangled_name;
	}
	void set_mangled_name(StringU8 name) override
	{
		m_mangled_name = std::move(name);
	}
	StringU8 get_param_name(size_t i) const override
	{
		return this->m_params[i]->get_ident().name;
	}
	IVar *get_param(size_t i) override
	{
		return this->m_params[i].get();
	}
	void codegen(CodeGenerator &g) override
	{
		this->codegen_func(g);
	}
	llvm::Value *gen_call(std::vector<llvm::Value *> args,
	                      CodeGenerator             &g) override;
};

struct StructBody : IBlock<IStructContent>
{
	std::vector<uptr<IStructContent>> m_content;

	void validate_types()
	{
		for (auto &&elem : m_content)
		{
			elem->validate_types();
		}
	}
};

struct StructDecl : Decl, IType
{
private:
	Env             *m_env;
	uptr<StructBody> m_body;
	SrcRange         m_range;
	Ident            m_ident;

public:
	StructDecl(Env             *env,
	           const SrcRange  &range,
	           Ident            ident,
	           uptr<StructBody> body);

	StringU8 dump_json() override
	{
		return fmt::format(
		    u8R"({{"obj":"StructDecl","ident":{},"body":{}}})",
		    m_ident.dump_json(),
		    m_body->dump_json());
	}
	SrcRange range() const override { return m_range; }
	Env     *env() const override { return m_env; }
	bool     can_accept(IType *iType) override;
	bool     equal(IType *iType) override;
	StringU8 get_type_name() override;
	void     validate_types() override;
};

struct Program : Ast
{
private:
	std::vector<uptr<Decl>> m_decls;
	uptr<Env>               m_root_env;
	Logger                 &logger;

public:
	explicit Program(std::vector<uptr<Decl>> decls,
	                 Logger                 &logger);

	StringU8 dump_json() override;
	SrcRange range() const override { return {}; }
	Env     *env() const override { return m_root_env.get(); }

	const std::vector<uptr<Decl>> &get_decls() const
	{
		return m_decls;
	}

	void codegen(CodeGenerator &g, bool &success);
	void codegen(CodeGenerator &g) override;

	void validate_types(bool &success);
};
} // namespace ast
} // namespace protolang