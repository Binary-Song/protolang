#pragma once
#include <format>
#include <utility>
#include "env.h"
#include "token.h"
namespace protolang
{

template <typename T>
inline std::string dump_json_for_vector_of_ptr(const std::vector<T> &data);
template <typename T>
inline std::string dump_json_for_vector(const std::vector<T> &data);

struct Ast
{
public:
	virtual ~Ast()                        = default;
	virtual std::string dump_json() const = 0;
};

struct Expr : public Ast
{
public:
	enum class ValueCat
	{
		Pending,
		Lvalue,
		Rvalue,
	} value_cat;

	bool is_lvalue() const { return value_cat == ValueCat::Lvalue; }
	bool is_rvalue() const { return value_cat == ValueCat::Rvalue; }

	explicit Expr(ValueCat valueCat = ValueCat::Pending)
	    : value_cat(valueCat)
	{}
	virtual ~Expr() = default;
};

struct ExprBinary : public Expr
{
public:
	uptr<Expr> left;
	uptr<Expr> right;
	Token      op;

public:
	ExprBinary(uptr<Expr> left, Token op, uptr<Expr> right)
	    : left(std::move(left))
	    , op(std::move(op))
	    , right(std::move(right))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "op":"{}", "left":{}, "right":{} }})",
		                   op.str_data,
		                   left->dump_json(),
		                   right->dump_json());
	}
};

struct ExprUnary : public Expr
{
public:
	uptr<Expr> right;
	Token      op;

public:
	ExprUnary(uptr<Expr> right, Token op)
	    : right(std::move(right))
	    , op(std::move(op))
	{}

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "op":"{}", "right":{} }})", op.str_data, right->dump_json());
	}
};

/// 括号
struct ExprGroup : public Expr
{
public:
	uptr<Expr> expr;

public:
	explicit ExprGroup(uptr<Expr> expr)
	    : expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "grouped":{} }})", expr->dump_json());
	}
};

struct ExprLiteral : public Expr
{
public:
	Token token;

public:
	explicit ExprLiteral(const Token &token)
	    : token(token)
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "str":"{}", "int":{}, "fp":{} }})",
		                   token.str_data,
		                   token.int_data,
		                   token.fp_data);
	}
};

struct Decl : public Ast
{
	std::string name;

	virtual uptr<NamedObject> declare() const = 0;

	Decl() = default;
	explicit Decl(string name)
	    : name(std::move(name))
	{}
};

struct DeclVar : public Decl
{
	std::string type;
	uptr<Expr>  init;

	DeclVar() = default;
	DeclVar(std::string name, std::string type, uptr<Expr> init)
	    : Decl(std::move(name))
	    , type(std::move(type))
	    , init(std::move(init))
	{}

	uptr<NamedObject> declare() const override
	{
		return uptr<NamedObject>(new NamedVar(name, type));
	}

	std::string dump_json() const override
	{
		return std::format(R"({{ "name":"{}", "type":"{}", "init":{} }})",
		                   name,
		                   type,
		                   init->dump_json());
	}
};

struct DeclParam : public Decl
{
	std::string type;

	DeclParam() = default;
	DeclParam(std::string name, std::string type)
	    : Decl(std::move(name))
	    , type(std::move(type))
	{}

	uptr<NamedObject> declare() const override
	{
		return uptr<NamedObject>(new NamedVar(name, type));
	}

	std::string dump_json() const override
	{
		return std::format(R"({{ "name":"{}", "type":"{}" }})", name, type);
	}
};

struct Stmt : public Ast
{};

struct StmtExpr : public Stmt
{
	uptr<Expr> expr;

	explicit StmtExpr(uptr<Expr> expr)
	    : expr(std::move(expr))
	{}

	std::string dump_json() const override
	{
		return std::format(R"({{ "expr":{} }})", expr->dump_json());
	}
};

struct CompoundStmtElem : public Ast
{

	explicit CompoundStmtElem(uptr<Stmt> stmt)
	    : _stmt(std::move(stmt))
	{}

	explicit CompoundStmtElem(uptr<DeclVar> var_decl)
	    : _var_decl(std::move(var_decl))
	{}

	Stmt    *stmt() const { return _stmt.get(); }
	DeclVar *var_decl() const { return _var_decl.get(); }

	std::string dump_json() const override
	{
		if (stmt())
			return stmt()->dump_json();
		if (var_decl())
			return var_decl()->dump_json();
		return "null";
	}

private:
	uptr<Stmt>    _stmt;
	uptr<DeclVar> _var_decl;
};

struct StmtCompound : public Stmt
{
	uptr<Env>                           env;
	std::vector<uptr<CompoundStmtElem>> elems;

	StmtCompound(uptr<Env> env)
	    : env(std::move(env))
	{}

	std::string dump_json() const override
	{
		return dump_json_for_vector_of_ptr(elems);
	}
};

struct DeclFunc : public Decl
{
	std::vector<uptr<DeclParam>> params;
	std::string                  return_type;
	uptr<StmtCompound>           body;

	DeclFunc() = default;
	DeclFunc(std::string                  name,
	         std::vector<uptr<DeclParam>> params,
	         std::string                  return_type,
	         uptr<StmtCompound>           body)
	    : Decl(std::move(name))
	    , params(std::move(params))
	    , return_type(std::move(return_type))
	    , body(std::move(body))
	{}

	uptr<NamedObject> declare() const override
	{
		NamedFunc *func = new NamedFunc();
		func->name      = name;
		for (auto &&param : params)
		{
			func->params.emplace_back(param->name, param->type);
		}
		func->return_type = return_type;
		return uptr<NamedObject>(func);
	}

	std::string dump_json() const override
	{
		return std::format(
		    R"({{ "name":"{}", "return_type":"{}", "body":{} }})",
		    name,
		    return_type,
		    body->dump_json());
	}
};

struct Program : public Ast
{
	std::vector<uptr<Decl>> decls;

	explicit Program(std::vector<uptr<Decl>> decls)
	    : decls(std::move(decls))
	{}

	std::string dump_json() const
	{
		return std::format(R"({{ "decls":[ {} ] }})",
		                   dump_json_for_vector_of_ptr(decls));
	}
};

template <typename T>
std::string dump_json_for_vector_of_ptr(const vector<T> &data)
{
	std::string json = "[";
	for (auto &&item : data)
	{
		json += item->dump_json();
		json += ",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += "]";
	return json;
}
template <typename T>
std::string dump_json_for_vector(const vector<T> &data)
{
	std::string json = "[";
	for (auto &&item : data)
	{
		json += item.dump_json();
		json += ",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += "]";
	return json;
}

} // namespace protolang