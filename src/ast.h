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
class TypeChecker;
class Logger;
class CodeGenerator;
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
	[[nodiscard]] const Ident &ident() const { return m_ident; }
	// 实现基类成员
	[[nodiscard]] std::string  dump_json() const override
	{
		return std::format(
		    R"({{"obj":"IdentTypeExpr","ident":{}}})",
		    m_ident.dump_json());
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range;
	}
	[[nodiscard]] Env *env() const override { return m_env; }
	[[nodiscard]] const IType *get_type() const override;
	void                       analyze_semantics() override;
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

	[[nodiscard]] virtual ValueCat get_value_cat() const
	{
		return ValueCat::Pending;
	}
	// 表达式默认的语义检查方法是计算一次类型
	void analyze_semantics() override
	{
		[[maybe_unused]] auto a = get_type();
	}
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
	[[nodiscard]] const std::unique_ptr<Expr> &get_left() const
	{
		return left;
	}
	[[nodiscard]] const std::unique_ptr<Expr> &get_right() const
	{
		return right;
	}
	[[nodiscard]] const Ident &get_op() const { return op; }
	[[nodiscard]] SrcRange     range() const override
	{
		return left->range() + right->range();
	}
	[[nodiscard]] Env *env() const override
	{
		assert(left->env() == right->env());
		return left->env();
	}
	[[nodiscard]] const IType *get_type() const override;
	[[nodiscard]] std::string  dump_json() const override
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

	[[nodiscard]] bool is_prefix() const { return prefix; }
	[[nodiscard]] const uptr<Expr> &get_operand() const
	{
		return operand;
	}
	[[nodiscard]] const Ident &get_op() const { return op; }
	[[nodiscard]] std::string  dump_json() const override
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
	[[nodiscard]] const IType *get_type() const override;
};

struct CallExpr : public Expr
{
	// 数据
private:
	uptr<Expr>              m_callee;
	std::vector<uptr<Expr>> m_args;
	const SrcRange         &m_src_rng;

public:
	CallExpr(const SrcRange         &src_rng,
	         uptr<Expr>              callee,
	         std::vector<uptr<Expr>> args)
	    : m_callee(std::move(callee))
	    , m_args(std::move(args))
	    , m_src_rng(src_rng)
	{}

	[[nodiscard]] const std::unique_ptr<Expr> &get_callee() const
	{
		return m_callee;
	}
	[[nodiscard]] const std::vector<uptr<Expr>> &get_args() const
	{
		return m_args;
	}
	[[nodiscard]] const SrcRange &get_range() const
	{
		return m_src_rng;
	}
	[[nodiscard]] std::vector<const IType *> arg_types() const;
	[[nodiscard]] std::string dump_json() const override
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
	[[nodiscard]] const IType *get_type() const override;
};

struct BracketExpr : public CallExpr
{
public:
	BracketExpr(const SrcRange         &src_rng,
	            uptr<Expr>              callee,
	            std::vector<uptr<Expr>> args)
	    : CallExpr(src_rng, std::move(callee), std::move(args))
	{}

	[[nodiscard]] std::string dump_json() const override
	{
		return std::format(
		    R"({{"obj":"BracketExpr","callee":{},"args":{}}})",
		    get_callee()->dump_json(),
		    dump_json_for_vector_of_ptr(get_args()));
	}

	// virtual uptr<Type> solve_type(TypeChecker *);
};

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

	[[nodiscard]] const uptr<Expr> &get_left() const
	{
		return m_left;
	}
	[[nodiscard]] const Ident &get_member() const
	{
		return m_member;
	}
	[[nodiscard]] std::string dump_json() const override
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
	[[nodiscard]] const IType *get_type() const override;
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

	[[nodiscard]] std::string dump_json() const override
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
	[[nodiscard]] Env *env() const override { return m_env; }
	[[nodiscard]] const IType *get_type() const override;
	[[nodiscard]] const Token &get_token() const
	{
		return m_token;
	}
	[[nodiscard]] llvm::Value *codegen(CodeGenerator &g) const;
};

struct IdentExpr : public Expr
{
private:
	Ident m_ident;
	Env  *m_env;

public:
	explicit IdentExpr(Env *env, Ident ident)
	    : m_env(env)
	    , m_ident(ident)
	{}

	[[nodiscard]] const Ident &ident() const { return m_ident; }

	[[nodiscard]] std::string dump_json() const override
	{
		return std::format("\"{}\"", m_ident.name);
	}

	[[nodiscard]] SrcRange range() const override
	{
		return m_ident.range;
	}
	[[nodiscard]] Env *env() const override { return m_env; }
	[[nodiscard]] const IType *get_type() const override;
	[[nodiscard]] llvm::Value *codegen(CodeGenerator &g) const;
};

struct Decl : Ast
{};

struct VarDecl : Decl, IVar
{
private:
	Ident          m_ident;
	uptr<TypeExpr> m_type;
	uptr<Expr>     m_init;

public:
	VarDecl(Ident ident, uptr<TypeExpr> type, uptr<Expr> init)
	    : m_ident(std::move(ident))
	    , m_type(std::move(type))
	    , m_init(std::move(init))
	{}
	[[nodiscard]] const Ident &get_ident() const override
	{
		return m_ident;
	}
	[[nodiscard]] const IType *get_type() const override
	{
		return m_type->get_type();
	}
	[[nodiscard]] const uptr<Expr> &get_init() const
	{
		return m_init;
	}
	[[nodiscard]] std::string dump_json() const override
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
	void analyze_semantics() override;
};

struct ParamDecl : Decl, IVar
{
private:
	Env           *m_env; // env在函数内部
	Ident          m_ident;
	uptr<TypeExpr> m_type; // 注意，它的env在函数外部

public:
	ParamDecl(Env *env, Ident ident, uptr<TypeExpr> type)
	    : m_env(env)
	    , m_ident(std::move(ident))
	    , m_type(std::move(type))
	{}

	[[nodiscard]] const Ident &get_ident() const override
	{
		return m_ident;
	}
	[[nodiscard]] const IType *get_type() const override
	{
		return m_type->get_type();
	}
	[[nodiscard]] std::string dump_json() const override
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
	[[nodiscard]] const std::unique_ptr<Expr> &get_expr() const
	{
		return m_expr;
	}
	[[nodiscard]] std::string dump_json() const override
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

	[[nodiscard]] std::string dump_json() const
	{
		return dump_json_for_vector_of_ptr(m_elems);
	}
	[[nodiscard]] SrcRange range() const { return m_range; }
	[[nodiscard]] Env     *env() const { return m_outer_env; }
	[[nodiscard]] Env *env_inner() const { return m_inner_env; }
	[[nodiscard]] Env *env_outer() const { return m_outer_env; }
	[[nodiscard]] const std::vector<uptr<Ast>> &elems() const
	{
		return m_elems;
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
	[[nodiscard]] std::string dump_json() const override
	{
		return Block::dump_json();
	}
	void analyze_semantics() override
	{
		Block::analyze_semantics();
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

public:
	FuncDecl() = default;
	FuncDecl(Env                         *env,
	         SrcRange                     range,
	         Ident                        ident,
	         std::vector<uptr<ParamDecl>> params,
	         uptr<TypeExpr>               return_type,
	         uptr<CompoundStmt>           body);

	[[nodiscard]] std::string dump_json() const override
	{
		return std::format(
		    R"({{"obj":"FuncDecl","ident":{},"return_type":{},"body":{}}})",
		    m_ident.dump_json(),
		    m_return_type->dump_json(),
		    m_body->dump_json());
	}
	[[nodiscard]] const Ident &get_ident() const
	{
		return m_ident;
	}
	[[nodiscard]] const IType *get_type() const override
	{
		return this;
	}
	[[nodiscard]] SrcRange range() const override
	{
		return m_range;
	}
	[[nodiscard]] Env *env() const override { return m_env; }

	[[nodiscard]] const IType *get_return_type() const override
	{
		return m_return_type->get_type();
	}
	[[nodiscard]] size_t get_param_count() const override
	{
		return m_params.size();
	}
	[[nodiscard]] const IType *get_param_type(
	    size_t i) const override
	{
		return m_params[i]->get_type();
	}
	[[nodiscard]] IFuncBody *get_body() const override
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
	[[nodiscard]] std::string dump_json() const override
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

	[[nodiscard]] std::string dump_json() const override
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
	bool        can_accept(const IType *iType) const override;
	bool        equal(const IType *iType) const override;
	std::string get_type_name() const override;
	void        analyze_semantics() override
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

	std::string dump_json() const
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