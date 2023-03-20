#include <iterator>
#include "env.h"
#include "ast.h"
#include "builtin.h"
#include "log.h"
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

bool Env::check_args(IFuncType                  *func,
                     const std::vector<IType *> &arg_types,
                     bool                        throw_error,
                     bool                        strict)
{
	if (func->get_param_count() != arg_types.size())
	{
		if (throw_error)
		{
			ErrorCallArgCountMismatch e;
			e.provided = arg_types.size();
			e.required = func->get_param_count();
			throw std::move(e);
		}
		else
			return false;
	}
	size_t argc = func->get_param_count();
	for (size_t i = 0; i < argc; i++)
	{
		auto p = func->get_param_type(i);
		auto a = arg_types[i];

		bool good = strict ? p->equal(a) : p->can_accept(a);
		if (!good)
		{
			if (throw_error)
			{
				ErrorCallTypeMismatch e;
				e.arg_index  = i;
				e.arg_type   = a->get_type_name();
				e.param_type = p->get_type_name();
				throw std::move(e);
			}
			return false;
		}
	}
	return true;
}

static std::vector<std::string> arg_type_names(
    const std::vector<IType *> &arg_types)
{
	std::vector<std::string> names;
	for (auto arg_type : arg_types)
	{
		names.push_back(arg_type->get_type_name());
	}
	return names;
};

IOp *Env::overload_resolution(
    const Ident                &func_ident,
    const std::vector<IType *> &arg_types)
{
	auto overloads = get<OverloadSet>(func_ident);

	std::vector<IOp *> fits;
	std::vector<IOp *> strict_fits;
	for (auto &&entity : *overloads)
	{
		std::cout << entity->dump_json() << std::endl;
		IOp *func = entity;
		if (check_args(func, arg_types, false, true))
		{
			fits.push_back(func);
			strict_fits.push_back(func);
		}
		else if (check_args(func, arg_types, false, false))
		{
			fits.push_back(func);
		}
	}

	// f=0, s=0 无
	// f=1, s=0 OK, f[0]
	// f=1, s=1 OK, f[0]
	// f>1, s=0 二义
	// f>1, s=1 OK, s[0]
	// f>1, s>1 二义

	if (fits.empty())
	{
		ErrorNoMatchingOverload e;
		e.overloads = overloads;
		e.arg_types = arg_type_names(arg_types);
		throw std::move(e);
	}

	if (fits.size() == 1)
	{
		return fits[0];
	}

	if (fits.size() > 1 && strict_fits.size() == 1)
	{
		return strict_fits[0];
	}

	logger.log(ErrorMultipleMatchingOverload(func_ident));
	throw ExceptionPanic();
}
void Env::add_to_overload_set(OverloadSet       *overloads,
                              IOp               *func,
                              const std::string &name)
{
	// 设置函数名
	func->set_mangled_name(get_full_qualified_name(name) + "#" +
	                       std::to_string(overloads->count()));
	overloads->add_func(func);
}

void Env::add(const std::string &name, IEntity *obj)
{
	bool name_clash = m_symbol_table.contains(name);
	if (auto func = dynamic_cast<IOp *>(obj))
	{
		if (name_clash)
		{ // 同名的玩意必须是函数重载集
			if (auto overloads = dynamic_cast<OverloadSet *>(
			        m_symbol_table.at(name)))
			{
				// ok: 有重名，但是函数
				add_to_overload_set(overloads, func, name);
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
			add_to_overload_set(overloads.get(), func, name);
			// 设置函数名
			func->set_mangled_name(name);
			m_symbol_table.insert({name, overloads.get()});
			m_owned_entities.push_back(std::move(overloads));
		}
	}
	else // 不是函数
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

std::string Env::dump_json()
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
	protolang::add_builtins(this);
	//	// 神说：
	//	// 要有int和float！
	//	add("int", BuiltInInt::get_instance());
	//	add("float", BuiltInFloat::get_instance());
	//	add("double", BuiltInDouble::get_instance());
	//	// 要有加法
	//	add("+", make_uptr(new BuiltInAdd<BuiltInInt>()));
	//	add("+", make_uptr(new BuiltInAdd<BuiltInDouble>()));
	//	add("+", make_uptr(new BuiltInAdd<BuiltInFloat>()));
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
OverloadSetConstIterator OverloadSet::begin() const
{
	return OverloadSetConstIterator{m_funcs.begin(), this};
}
OverloadSetConstIterator OverloadSet::end() const
{
	return {};
}

OverloadSetConstIterator OverloadSet::cbegin() const
{
	return OverloadSetConstIterator{m_funcs.begin(), this};
}
OverloadSetConstIterator OverloadSet::cend() const
{
	return {};
}

std::string OverloadSet::dump_json()
{
	std::vector<IOp *> all;
	for (auto iter = begin(); iter != end(); ++iter)
	{
		auto f = *iter;
		all.push_back(f);
	}
	return std::format(R"({{"obj":"OverloadSet","funcs":{}}})",
	                   dump_json_for_vector_of_ptr(all));
}
size_t OverloadSet::count() const
{
	if (m_next)
		return this->m_funcs.size() + m_next->count();
	return this->m_funcs.size();
}

/*
 *
 *                  OVERLOAD SET ITER
 *
 * */

OverloadSetIterator::OverloadSetIterator(
    std::vector<IOp *>::iterator iter, OverloadSet *curr)
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
	return is_end == iter.is_end && m_iter == iter.m_iter;
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
    std::vector<IOp *>::const_iterator iter,
    const OverloadSet                 *curr)
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
	return is_end == iter.is_end && m_iter == iter.m_iter;
}
bool OverloadSetConstIterator::operator!=(
    const OverloadSetConstIterator &iter) const
{
	return !(*this == iter);
}
} // namespace protolang