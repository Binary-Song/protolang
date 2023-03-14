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
	auto overloads = get<OverloadSet>(func_ident);
	std::vector<IFunc *> fits;
	for (auto &&entity : *overloads)
	{
		IFunc *func = entity;
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
void Env::add(const std::string &name, IEntity *obj)
{
	bool name_clash = m_symbol_table.contains(name);
	if (auto func = dynamic_cast<IFunc *>(obj))
	{
		if (name_clash)
		// 同名的玩意必须是函数重载集
		{
			if (auto overloads = dynamic_cast<OverloadSet *>(
			        m_symbol_table.at(name)))
			{
				// ok: 有重名，但是函数
				overloads->add_func(func);
			}
			else
			{
				// 重定义了
				auto i = m_symbol_table.at(name);
				logger.log(ErrorSymbolRedefinition(
				    name, try_get_range(i), try_get_range(obj)));
				throw ExceptionPanic();
			}
		}
		else
		{ // 没有重名
			auto overloads =
			    make_uptr<OverloadSet>(new OverloadSet{
			        m_parent ? m_parent->get_overload_set(name)
			                 : nullptr});
			overloads->add_func(func);
			m_symbol_table.insert({name, overloads.get()});
			m_owned_entities.push_back(std::move(overloads));
		}
	}
	else
	{
		if (name_clash)
		{
			// 重定义了
			auto i = m_symbol_table.at(name);
			logger.log(ErrorSymbolRedefinition(
			    name, try_get_range(i), try_get_range(obj)));
			throw ExceptionPanic();
		}
		else
		{
			m_symbol_table.insert({name, obj});
		}
	}
}

std::string Env::dump_json() const
{
	std::vector<IEntity *> vals;
	for (auto &&[_, value] : m_symbol_table)
	{
		vals.push_back(value);
	}
	return std::format(
	    R"({{"obj":"Env","this":{},"sub":{}}})",
	    dump_json_for_vector_of_ptr(vals),
	    dump_json_for_vector_of_ptr(this->m_subenvs));
}

void Env::add_built_in_facility()
{
	// 神说：
	// 要有int和float！
	add("int", BuiltInInt::get_instance());
	add("float", BuiltInFloat::get_instance());
	add("double", BuiltInDouble::get_instance());
	// 要有加法
	add("+", make_uptr(new BuiltInAdd<BuiltInInt>()));
	add("+", make_uptr(new BuiltInAdd<BuiltInDouble>()));
	add("+", make_uptr(new BuiltInAdd<BuiltInFloat>()));
}

/*
 *
 *                  OVERLOAD SET
 *
 * */

OverloadSetIterator OverloadSet::begin()
{
	return OverloadSetIterator{m_funcs.begin(), this};
}
OverloadSetIterator OverloadSet::end()
{
	return {};
}
std::string OverloadSet::dump_json() const
{
	std::vector<const IFunc *> all;
	for (auto &&f : *this)
	{
		all.push_back(f);
	}
	return std::format(R"({{"obj":"OverloadSet","funcs":{}}})",
	                   dump_json_for_vector_of_ptr(all));
}

/*
 *
 *                  OVERLOAD SET ITER
 *
 * */
OverloadSetIterator::OverloadSetIterator(
    std::vector<IFunc *>::iterator iter, OverloadSet *curr)
    : m_iter(iter)
    , m_curr_set(curr)
{}
void OverloadSetIterator::jump_until_valid()
{
	// 当前的end和下一个的begin是等价的，
	// 我们比较嫌弃处在某个end上，所以
	// 在等价类中移动
	// 直到不在end上
	// 更重要的是，该函数更新is_end状态！！
	while (m_iter == m_curr_set->m_funcs.end())
	{
		if (m_curr_set->m_next)
		{ // 有路可走
			m_curr_set = m_curr_set->m_next;
			m_iter     = m_curr_set->m_funcs.begin();
		}
		else
		{ // 无路可走
			is_end = true;
			return;
		}
	}
	is_end = false;
}
OverloadSetIterator &OverloadSetIterator::operator++()
{
	jump_until_valid();
	if (!is_end)
	{
		++m_iter;
		jump_until_valid();
		return *this;
	}
	else
		return *this;
}
bool OverloadSetIterator::operator==(
    const OverloadSetIterator &iter) const
{
	if (is_end && iter.is_end)
		return true;
	return m_iter == iter.m_iter;
}
bool OverloadSetIterator::operator!=(
    const OverloadSetIterator &iter) const
{
	return !(*this == iter);
}
/*
 *
 *              OVERLOAD SET CONST ITER
 *
 * */
OverloadSetConstIterator::OverloadSetConstIterator(
    std::vector<IFunc *>::const_iterator iter,
    const OverloadSet                   *curr)
    : m_iter(iter)
    , m_curr_set(curr)
{}
void OverloadSetConstIterator::jump_until_valid()
{
	// 当前的end和下一个的begin是等价的，
	// 我们比较嫌弃处在某个end上，所以
	// 在等价类中移动
	// 直到不在end上
	// 更重要的是，该函数更新is_end状态！！
	while (m_iter == m_curr_set->m_funcs.end())
	{
		if (m_curr_set->m_next)
		{ // 有路可走
			m_curr_set = m_curr_set->m_next;
			m_iter     = m_curr_set->m_funcs.begin();
		}
		else
		{ // 无路可走
			is_end = true;
			return;
		}
	}
	is_end = false;
}
OverloadSetConstIterator &OverloadSetConstIterator::operator++()
{
	jump_until_valid();
	if (!is_end)
	{
		++m_iter;
		jump_until_valid();
		return *this;
	}
	else
		return *this;
}
bool OverloadSetConstIterator::operator==(
    const OverloadSetConstIterator &iter) const
{
	if (is_end && iter.is_end)
		return true;
	return m_iter == iter.m_iter;
}
bool OverloadSetConstIterator::operator!=(
    const OverloadSetConstIterator &iter) const
{
	return !(*this == iter);
}
} // namespace protolang