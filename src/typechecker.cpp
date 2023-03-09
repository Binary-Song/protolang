#include "typechecker.h"
#include "ast.h"
#include "type.h"

namespace protolang
{

void TypeChecker::check()
{

	for (auto &&decl : program->decls)
	{
		try
		{
			decl->check_type(this);
		}
		catch (const ExceptionPanic &)
		{}
	}
}

bool TypeChecker::check_args(NamedFunc                 *func,
                             const std::vector<Type *> &arg_types)
{
	if (func->params.size() != arg_types.size())
		return false;
	size_t argc = func->params.size();
	for (size_t i = 0; i < argc; i++)
	{
		auto p = func->params[i].type->type(this);
		auto a = arg_types[i];
		if (!p->can_accept(a))
		{
			return false;
		}
	}
	return true;
}

NamedFunc *TypeChecker::overload_resolution(
    Env *env, const Ident &func_ident, const std::vector<Type *> &arg_types)
{
	auto                     overloads = env->get_all(func_ident);
	std::vector<NamedFunc *> fits;
	for (auto &&entity : overloads)
	{
		assert(dynamic_cast<NamedFunc *>(entity));
		NamedFunc *func = dynamic_cast<NamedFunc *>(entity);
		if (check_args(func, arg_types))
		{
			fits.push_back(func);
		}
	}
	if (fits.size() == 0)
	{
		logger.log(ErrorNoMatchingOverload(func_ident));
		throw ExceptionPanic();
	}
	if (fits.size() > 1)
	{
		logger.log(ErrorMultipleMatchingOverload(func_ident));
		throw ExceptionPanic();
	}
	return fits[0];
}

void TypeChecker::add_builtin_facility()
{
	// todo: name添加顺序的问题，parse比他还快！

	// 创建 int double
	add_builtin_type("Int");
	add_builtin_type("Double");

	// 创建 加法
	add_builtin_op(
	    "+", dynamic_cast<IdentTypeExpr *>(builtin_type_exprs["Int"].get()));
	add_builtin_op(
	    "+", dynamic_cast<IdentTypeExpr *>(builtin_type_exprs["Double"].get()));
}

void TypeChecker::add_builtin_op(const std::string &op,
                                 IdentTypeExpr     *operand_type)
{

	auto props       = NamedEntityProperties();
	auto ident       = Ident(op, {});
	auto return_type = operand_type;
	auto params      = {
        NamedVar{{}, Ident{"lhs", {}}, operand_type},
        NamedVar{{}, Ident{"rhs", {}}, operand_type},
    };

	auto new_func      = new NamedFunc(props, ident, return_type, params);
	auto new_func_uptr = uptr<NamedFunc>(new_func);
	root_env->add_func(std::move(new_func_uptr));
}

void TypeChecker::add_builtin_type(const std::string &name)
{
	// add types
	uptr<NamedType> named_type = std::make_unique<NamedType>(
	    NamedEntityProperties{}, Ident(name, Pos2DRange{}));
	root_env->add_non_func(std::move(named_type));

	uptr<IdentTypeExpr> type_expr =
	    std::make_unique<IdentTypeExpr>(root_env, Ident{name, {}});
	this->builtin_type_exprs[name] = (std::move(type_expr));
}
Type *TypeChecker::get_builtin_type(const std::string &name)
{
	return builtin_type_exprs.at(name)->type(this);
}
/*
 *
 *             AST 相关虚函数实现
 *
 */

void VarDecl::check_type(TypeChecker *tc)
{
	// 检查 init 和 声明的类别兼容
	if (!this->type->type(tc)->can_accept(this->init->type(tc)))
	{
		tc->logger.log(ErrorTypeMismatch(init->type(tc)->full_name(),
		                                 Pos2DRange{},
		                                 type->type(tc)->full_name(),
		                                 Pos2DRange{}));
	}
}

uptr<Type> BinaryExpr::solve_type(TypeChecker *tc)
{
	auto       lhs_type = this->left->type(tc); 
	auto       rhs_type = this->right->type(tc);  
	NamedFunc *func = tc->overload_resolution(env, op, {lhs_type, rhs_type});
	return func->return_type->type(tc)->clone();
}

uptr<Type> IdentTypeExpr::solve_type(TypeChecker *tc)
{
	NamedEntity *ent = this->env->get_one(ident);
	if (ent->named_entity_type() != NamedEntityType::NamedType)
	{
		// 存在这个名字，但根本不是个类型，你把它当类型用是吧TNND
		tc->logger.log(ErrorSymbolIsNotAType(this->ident));
		throw ExceptionPanic();
	}
	// 是type
	NamedType *type = dynamic_cast<NamedType *>(ent);
	return std::make_unique<IdentType>(type);
}
uptr<Type> LiteralExpr::solve_type(TypeChecker *tc)
{
	if (token.type == Token::Type::Int)
	{
		return tc->get_builtin_type("Int")->clone();
	}
	else if (token.type == Token::Type::Fp)
	{
		return tc->get_builtin_type("Double")->clone();
	}
	assert(false); // 没有这种literal
	return nullptr;
}
} // namespace protolang
