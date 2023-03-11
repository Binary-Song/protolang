#include "env.h"
#include "ast.h"
#include "util.h"
namespace protolang
{


//
// void Env::add_builtin_op(const std::string &op,
//                         const std::string &type)
//{
//	auto param1 = make_uptr(new ParamDecl(
//	    this,
//	    {},
//	    Ident{"lhs", {}},
//	    make_uptr(new IdentTypeExpr(this, Ident{type, {}}))));
//
//	auto param2 = make_uptr(new ParamDecl(
//	    this,
//	    {},
//	    Ident{"rhs", {}},
//	    make_uptr(new IdentTypeExpr(this, Ident{type, {}}))));
//
//	std::vector<uptr<ParamDecl>> params;
//	params.push_back(std::move(param1));
//	params.push_back(std::move(param2));
//
//	auto ident = Ident(op, {});
//	auto return_type =
//	    make_uptr(new IdentTypeExpr(this, Ident{type, {}}));
//
//	std::vector<uptr<CompoundStmtElem>> body_elems;
//	auto body_env = make_uptr(new Env(this, logger));
//
//	auto body = make_uptr(new CompoundStmt(
//	    this, {}, std::move(body_env), std::move(body_elems)));
//
//	auto func_decl =
//	    make_uptr(new FuncDecl(this,
//	                           {},
//	                           Ident{"+", {}},
//	                           std::move(params),
//	                           std::move(return_type),
//	                           std::move(body)));
//
//	func_decl->add_to_env({}, this);
//}
//
// void Env::add_builtin_type(const std::string &name)
//{
//	// add types
//	uptr<NamedType> named_type = std::make_unique<NamedType>(
//	    NamedEntityProperties{}, Ident(name, SrcRange{}));
//	this->add_non_func(std::move(named_type));
//
//	uptr<IdentTypeExpr> type_expr =
//	    std::make_unique<IdentTypeExpr>(this, Ident{name, {}});
//	this->builtin_exprs[name] = (std::move(type_expr));
//}
bool Env::check_args(IFuncType                  *func,
                     const std::vector<IType *> &arg_types,
                     bool                        throw_error)
{
	if (func->get_param_count() != arg_types.size())
		return false;
	size_t argc = func->get_param_count();
	for (size_t i = 0; i < argc; i++)
	{
		auto p = func->get_param_type(i);
		auto a = arg_types[i];
		if (!p->can_accept(a))
		{
			if (throw_error)
			{
				logger.log(ErrorTypeMismatch(a->get_type_name(),
				                             {},
				                             p->get_type_name(),
				                             {}));
				throw ExceptionPanic();
			}
			return false;
		}
	}
	return true;
}

IFunc *Env::overload_resolution(
    const Ident                &func_ident,
    const std::vector<IType *> &arg_types)
{
	auto                 overloads = get_all(func_ident);
	std::vector<IFunc *> fits;
	for (auto &&entity : overloads)
	{
		assert(dynamic_cast<IFunc *>(entity));
		IFunc *func = dynamic_cast<IFunc *>(entity);
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

} // namespace protolang