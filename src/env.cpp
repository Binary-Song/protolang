#include "env.h"
#include "ast.h"
#include "builtin.h"
#include "util.h"
namespace protolang
{
static SrcRange try_get_range(const IEntity *entity)
{
	if (auto ast = dynamic_cast<const ast::Ast *>(entity))
	{
		return ast->range();
	}
	return {};
}

bool Env::check_args(const IFuncType                  *func,
                     const std::vector<const IType *> &arg_types,
                     bool throw_error)
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
    const Ident                      &func_ident,
    const std::vector<const IType *> &arg_types)
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
void Env::add_non_func(const std::string &name, IEntity *obj)
{
	// 禁止用add添加函数，请用add_overload替代
	assert(dynamic_cast<IFunc *>(obj) == nullptr);
	if (m_symbol_table.contains(name))
	{
		// 重定义了
		auto [begin, _] = m_symbol_table.equal_range(name);
		logger.log(
		    ErrorSymbolRedefinition(name,
		                            try_get_range(begin->second),
		                            try_get_range(obj)));
		throw ExceptionPanic();
	}
	m_symbol_table.insert({name, obj});
	m_entities.push_back(obj);
}
void Env::add_alias(const Ident &alias, const Ident &target_name)
{
	// todo: 不许创建除了type-alias以外的alias。
	// 目前能为任何无歧义的名字创建别名
	IEntity *target = get_one(target_name);
	if (m_symbol_table.contains(alias.name))
	{
		auto [begin, _] = m_symbol_table.equal_range(alias.name);
		logger.log(
		    ErrorSymbolRedefinition(alias.name,
		                            try_get_range(begin->second),
		                            alias.range));
		throw ExceptionPanic();
	}
	m_symbol_table.insert({alias.name, target});
}
void Env::add_func(const std::string &name, IFunc *func)
{
	// todo: 检查overload不能互相冲突，造成二义性调用
	// 函数名不能和变量名、类型名等冲突，但函数名可以重复
	if (m_symbol_table.contains(name))
	{
		auto [val_begin, val_end] =
		    m_symbol_table.equal_range(name);
		// 所有同名的玩意必须只能是函数
		for (auto iter = val_begin; iter != val_end; ++iter)
		{
			auto same_name_entity = iter->second;
			if (!dynamic_cast<IFunc *>(same_name_entity))
			{
				logger.log(ErrorSymbolRedefinition(
				    name,
				    try_get_range(same_name_entity),
				    try_get_range(func)));
				throw ExceptionPanic();
			}
		}
		// ok: 有重名，但是函数
	}
	m_symbol_table.insert({name, func});
	m_entities.push_back(func); // 不可早move!
}

std::vector<IEntity *> Env::get_all(const Ident &ident) const
{
	std::vector<IEntity *> result;
	std::string            name = ident.name;
	auto [begin, end] = m_symbol_table.equal_range(name);
	for (auto iter = begin; iter != end; ++iter)
	{
		result.push_back(iter->second);
	}
	// 如果有爹，加上爹的
	if (m_parent)
	{
		auto parents = m_parent->get_all(ident);
		result.insert(
		    result.end(), parents.begin(), parents.end());
	}
	return result;
}
std::string Env::dump_json() const
{
	std::vector<IEntity *> vals;
	for (auto &&kv : m_symbol_table)
	{
		vals.push_back(kv.second);
	}
	if (m_parent)
		return std::format(
		    R"({{"obj":"Env","this":{},"parent":{}}})",
		    dump_json_for_vector_of_ptr(vals),
		    m_parent->dump_json());
	else
		return std::format(R"({{ "this": {} }})",
		                   dump_json_for_vector_of_ptr(vals));
}

void Env::add_built_in_facility()
{
	// 神说：
	// 要有int和float！
	add_non_func("int", BuiltInInt::get_instance());
	add_non_func("float", BuiltInFloat::get_instance());
	// 要有加法
	add_func("+", make_uptr(new BuiltInIntAdd()));
}

} // namespace protolang