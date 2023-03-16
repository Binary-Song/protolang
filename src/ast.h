#pragma once
#include <cassert>
#include <format>
#include <functional>
#include <utility>
#include <vector>
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
struct Ast : virtual IJsonDumper
{
	// 函数
public:
	[[nodiscard]] Env *root_env() const;

	// 虚函数
public:
	virtual ~Ast()                                     = default;
	[[nodiscard]] virtual SrcRange range() const       = 0;
	[[nodiscard]] virtual Env     *env() const         = 0;
	virtual void                   analyze_semantics() = 0;
};

// 类型表达式，这是类型的引用，并不是真正的类型声明，抽象类
struct TypeExpr : Ast, ITyped
{};

// 类型标识符
struct IdentTypeExpr : TypeExpr
{
	// 数据
private:
	Ident m_ident;
	Env  *m_env;
	// 函数
public:
	explicit IdentTypeExpr(Env *env, Ident ident);
	[[nodiscard]] Ident       ident() const { return m_ident; }
	// 实现基类成员
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"IdentTypeExpr","ident":{}}})",
		    m_ident.dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range;
	}
	[[nodiscard]] Env   *env() const override { return m_env; }
	[[nodiscard]] IType *get_type() override;
	void                 analyze_semantics() override;
};

// 表达式，抽象类
struct Expr : public Ast, public ITyped
{
	// 类
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	};

	[[nodiscard]] virtual ValueCat get_stack_addr_cat() const
	{
		return ValueCat::Pending;
	}
	// 表达式默认的语义检查方法是计算一次类型
	void analyze_semantics() override;

	virtual llvm::Value *codegen(CodeGenerator &g) = 0;
};

// 二元运算表达式
struct BinaryExpr : public Expr
{
	// 数据
private:
	uptr<Expr> left;
	uptr<Expr> right;
	Ident      op;

	// 函数
public:
	BinaryExpr(uptr<Expr> left, Ident op, uptr<Expr> right)
	    : left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}
	[[nodiscard]] Expr    *get_left() { return left.get(); }
	[[nodiscard]] Expr    *get_right() { return right.get(); }
	[[nodiscard]] Ident    get_op() const { return op; }
	[[nodiscard]] SrcRange range() const override
	{
		return left->range() + right->range();
	}
	[[nodiscard]] Env *env() const override
	{
		assert(left->env() == right->env());
		return left->env();
	}
	[[nodiscard]] IType      *get_type() override;
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"BinaryExpr","op":{},"lhs":{},"rhs":{}}})",
		    op.dump_json(),
		    left->dump_json(),
		    right->dump_json());
	}
};

struct UnaryExpr : public Expr
{
	// 数据
public:
	bool       prefix;
	uptr<Expr> operand;
	Ident      op;

	// 函数
public:
	UnaryExpr(bool prefix, uptr<Expr> operand, Ident op)
	    : prefix(prefix)
	    , operand(std::move(operand))
	    , op(std::move(op))
	{}

	[[nodiscard]] bool  is_prefix() const { return prefix; }
	[[nodiscard]] Expr *get_operand() { return operand.get(); }
	[[nodiscard]] Ident get_op() const { return op; }
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"UnaryExpr","op":{},"oprd":{}}})",
		    op.dump_json(),
		    operand->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return op.range + operand->range();
	}
	[[nodiscard]] Env *env() const override
	{
		return operand->env();
	}
	[[nodiscard]] IType *get_type() override;
};

struct CallExpr : public Expr
{
	// 数据
protected:
	uptr<Expr>              m_callee;
	std::vector<uptr<Expr>> m_args;
	SrcRange                m_src_rng;

public:
	CallExpr(const SrcRange         &src_rng,
	         uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args)
	    : m_callee(std::move(callee))
	    , m_args(std::move(args))
	    , m_src_rng(src_rng)
	{}

	[[nodiscard]] Expr *get_callee() const
	{
		return m_callee.get();
	}
	[[nodiscard]] size_t get_arg_count() const
	{
		return m_args.size();
	}
	[[nodiscard]] Expr *get_arg(size_t index) const
	{
		return m_args[index].get();
	}
	[[nodiscard]] SrcRange get_range() const
	{
		return m_src_rng;
	}
	[[nodiscard]] IType *get_arg_type(size_t index);
	[[nodiscard]] std::vector<IType *> get_arg_types()
	{
		std::vector<IType *> types;
		for (size_t i = 0; i < get_arg_count(); i++)
		{
			types.push_back(get_arg_type(i));
		}
		return types;
	}
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"CallExpr","callee":{},"args":{}}})",
		    m_callee->dump_json(),
		    dump_json_for_vector_of_ptr(m_args));
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_src_rng;
	}
	[[nodiscard]] Env *env() const override
	{
		return m_callee->env();
	}
	[[nodiscard]] IType *get_type() override;
};

struct BracketExpr : public CallExpr
{
public:
	BracketExpr(const SrcRange         &src_rng,
	            uptr<Expr>              callee,
	            std::vector<uptr<Expr>> args)
	    : CallExpr(src_rng, std::move(callee), std::move(args))
	{}

	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"BracketExpr","callee":{},"args":{}}})",
		    get_callee()->dump_json(),
		    dump_json_for_vector_of_ptr(m_args));
	}

	// virtual uptr<Type> solve_type(TypeChecker *);
};
struct IdentExpr;
struct MemberAccessExpr : public Expr
{
	// 数据
public:
	uptr<Expr> m_left;
	Ident      m_member;

	// 函数
public:
	MemberAccessExpr(uptr<Expr> left, Ident member)
	    : m_left(std::move(left))
	    , m_member(std::move(member))
	{}

	[[nodiscard]] Expr       *get_left() { return m_left.get(); }
	[[nodiscard]] Ident       get_member() { return m_member; }
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"MemberAccessExpr","left":{},"member":{}}})",
		    m_left->dump_json(),
		    m_member.dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_left->range() + m_member.range;
	}
	[[nodiscard]] Env *env() const override
	{
		return m_left->env();
	}
	[[nodiscard]] IType *get_type() override;
};

struct LiteralExpr : public Expr
{
public:
	Token m_token;
	Env  *m_env;

public:
	explicit LiteralExpr(Env *env, Token token)
	    : m_env(env)
	    , m_token(std::move(token))
	{}

	[[nodiscard]] std::string dump_json() override
	{
		return std::format("\"{}/{}/{}\"",
		                   m_token.str_data,
		                   m_token.int_data,
		                   m_token.fp_data);
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_token.range();
	}
	[[nodiscard]] Env   *env() const override { return m_env; }
	[[nodiscard]] IType *get_type() override;
	[[nodiscard]] Token  get_token() const { return m_token; }
	[[nodiscard]] llvm::Value *codegen(CodeGenerator &g);
};

struct IdentExpr : public Expr
{
private:
	Ident  m_ident;
	Env   *m_env;
	IType *m_type = nullptr;

public:
	explicit IdentExpr(Env *env, Ident ident)
	    : m_env(env)
	    , m_ident(ident)
	{}

	[[nodiscard]] Ident ident() const { return m_ident; }

	[[nodiscard]] std::string dump_json() override
	{
		return std::format("\"{}\"", m_ident.name);
	}

	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range;
	}
	[[nodiscard]] Env   *env() const override { return m_env; }
	[[nodiscard]] IType *get_type() override;
	[[nodiscard]] void   set_type(IType *type);
	[[nodiscard]] llvm::Value *codegen(CodeGenerator &g);
};

struct Decl : Ast
{};

struct VarDecl : Decl, IVar
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
	[[nodiscard]] Ident get_ident() const override
	{
		return m_ident;
	}
	[[nodiscard]] IType *get_type() override
	{
		return m_type->get_type();
	}
	[[nodiscard]] Expr *get_init() override
	{
		return m_init.get();
	}
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"VarDecl","ident":{},"type":{},"init":{}}})",
		    m_ident.dump_json(),
		    m_type->dump_json(),
		    m_init->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range + m_init->range();
	}
	[[nodiscard]] Env *env() const override
	{
		return m_type->env();
	}
	void              analyze_semantics() override;
	llvm::AllocaInst *get_stack_addr() const override
	{
		return m_value;
	}
	void set_stack_addr(llvm::AllocaInst *value) override
	{
		m_value = value;
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

	[[nodiscard]] Ident get_ident() const override
	{
		return m_ident;
	}
	[[nodiscard]] Expr  *get_init() override { return nullptr; }
	[[nodiscard]] IType *get_type() override
	{
		return m_type->get_type();
	}
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"ParamDecl","ident":{},"type":{} }})",
		    m_ident.dump_json(),
		    m_type->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range + m_type->range();
	}
	[[nodiscard]] Env *env() const override { return m_env; }
	void               analyze_semantics() override
	{
		this->m_type->analyze_semantics();
	}
	llvm::AllocaInst *get_stack_addr() const override
	{
		return m_value;
	}
	void set_stack_addr(llvm::AllocaInst *value) override
	{
		m_value = value;
	}
};

struct Stmt : public Ast
{};

struct ExprStmt : public Stmt
{
private:
	uptr<Expr> m_expr;

public:
	explicit ExprStmt(uptr<Expr> expr)
	    : m_expr(std::move(expr))
	{}
	[[nodiscard]] Expr       *get_expr() { return m_expr.get(); }
	[[nodiscard]] std::string dump_json() override
	{
		return std::format(R"({{"obj":"ExprStmt","expr":{}}})",
		                   m_expr->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_expr->range();
	}
	[[nodiscard]] Env *env() const override
	{
		return m_expr->env();
	}
	void analyze_semantics() override
	{
		m_expr->analyze_semantics();
	}
};

struct Block
{
protected:
	Env                   *m_outer_env;
	Env                   *m_inner_env;
	SrcRange               m_range;
	// FuncBody: statement或var_decl
	// StructBody: var_decl或func_decl
	std::vector<uptr<Ast>> m_elems;

public:
	Block(Env                   *outer_env,
	      SrcRange               range,
	      std::vector<uptr<Ast>> elems);

	[[nodiscard]] std::string dump_json()
	{
		return dump_json_for_vector_of_ptr(m_elems);
	}
	[[nodiscard]] SrcRange range() const { return m_range; }
	[[nodiscard]] Env     *env() const { return m_outer_env; }
	[[nodiscard]] Env *env_inner() const { return m_inner_env; }
	[[nodiscard]] Env *env_outer() const { return m_outer_env; }
	[[nodiscard]] size_t elem_count() const
	{
		return m_elems.size();
	}
	[[nodiscard]] Ast *get_elem(size_t index)
	{
		return m_elems[index].get();
	};
	[[nodiscard]] void add_elem(uptr<Ast> e)
	{
		m_elems.push_back(std::move(e));
	}
	[[nodiscard]] void set_range(const SrcRange &range)
	{
		this->m_range = range;
	}
	void analyze_semantics()
	{
		for (auto &&elem : m_elems)
		{
			elem->analyze_semantics();
		}
	}
};

struct CompoundStmt : Block, Stmt, IFuncBody
{
	using Block::Block;

	[[nodiscard]] SrcRange range() const override
	{
		return Block::range();
	}
	[[nodiscard]] Env *env() const override
	{
		return Block::env();
	}
	[[nodiscard]] std::string dump_json() override
	{
		return Block::dump_json();
	}
	void analyze_semantics() override
	{
		Block::analyze_semantics();
	}
	void codegen(CodeGenerator &g) override
	{

	}
};

struct FuncDecl : Decl, IFunc
{
private:
	Env                         *m_env;
	SrcRange                     m_range;
	Ident                        m_ident;
	std::vector<uptr<ParamDecl>> m_params;
	uptr<TypeExpr>               m_return_type;
	uptr<CompoundStmt>           m_body;
	std::string                  m_mangled_name;

public:
	FuncDecl() = default;
	FuncDecl(Env                         *env,
	         SrcRange                     range,
	         Ident                        ident,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"FuncDecl","ident":{},"return_type":{},"body":{}}})",
		    m_ident.dump_json(),
		    m_return_type->dump_json(),
		    m_body->dump_json());
	}
	[[nodiscard]] Ident    get_ident() const { return m_ident; }
	[[nodiscard]] IType   *get_type() override { return this; }
	[[nodiscard]] SrcRange range() const override
	{
		return m_range;
	}
	[[nodiscard]] Env *env() const override { return m_env; }

	[[nodiscard]] IType *get_return_type() override
	{
		return m_return_type->get_type();
	}
	[[nodiscard]] size_t get_param_count() const override
	{
		return m_params.size();
	}
	[[nodiscard]] IType *get_param_type(size_t i) override
	{
		return m_params[i]->get_type();
	}
	[[nodiscard]] IFuncBody *get_body() override
	{
		return m_body.get();
	}
	// todo: params 如果有默认值，可能还得check一下
	// todo: body 里的 return 必须 check
	void analyze_semantics() override
	{
		for (auto &&p : m_params)
		{
			p->analyze_semantics();
		}
		m_return_type->analyze_semantics();
		m_body->analyze_semantics();
	}
	std::string get_mangled_name() const override
	{
		return m_mangled_name;
	}
	void set_mangled_name(std::string name) override
	{
		m_mangled_name = std::move(name);
	}
	std::string get_param_name(size_t i) const override
	{
		return this->m_params[i]->get_ident().name;
	}
	IVar *get_param(size_t i) override
	{
		return this->m_params[i].get();
	}
};

struct StructBody : Block, Ast
{
	using Block::Block;

	[[nodiscard]] SrcRange range() const override
	{
		return Block::range();
	}
	[[nodiscard]] Env *env() const override
	{
		return Block::env();
	}
	[[nodiscard]] std::string dump_json() override
	{
		return Block::dump_json();
	}
	void analyze_semantics() override
	{
		Block::analyze_semantics();
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

	[[nodiscard]] std::string dump_json() override
	{
		return std::format(
		    R"({{"obj":"StructDecl","ident":{},"body":{}}})",
		    m_ident.dump_json(),
		    m_body->dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_range;
	}
	[[nodiscard]] Env *env() const override { return m_env; }
	bool               can_accept(IType *iType) override;
	bool               equal(IType *iType) override;
	std::string        get_type_name() override;
	void               analyze_semantics() override
	{
		m_body->analyze_semantics();
	}
};

struct Program : public Ast
{
private:
	std::vector<uptr<Decl>> m_decls;
	uptr<Env>               m_root_env;

public:
	explicit Program(std::vector<uptr<Decl>> decls,
	                 Logger                 &logger);

	std::string dump_json()
	{
		return std::format(R"({{"obj":"Program","decls":[{}]}})",
		                   dump_json_for_vector_of_ptr(m_decls));
	}
	SrcRange range() const override { return {}; }
	Env     *env() const override { return m_root_env.get(); }

	const std::vector<uptr<Decl>> &get_decls() const
	{
		return m_decls;
	}
	void analyze_semantics() override
	{
		for (auto &&decl : m_decls)
		{
			decl->analyze_semantics();
		}
	}
};
} // namespace ast
} // namespace protolang