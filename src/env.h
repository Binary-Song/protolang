#pragma once
#include <cassert>
#include <concepts>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "entity_system.h"
#include "logger.h"
#include "token.h"
#include "typedef.h"
#include "util.h"
namespace protolang
{

class OverloadSetIterator;

// 重载函数的集合，请用iterator或range-for访问
class OverloadSet : public IEntity
{
	friend class OverloadSetIterator;
	friend class OverloadSetConstIterator;
	std::vector<IFunc *> m_funcs;
	OverloadSet         *m_next = nullptr;

public:
	explicit OverloadSet(OverloadSet *next)
	    : m_next(next)
	{}
	void   add_func(IFunc *func) { m_funcs.push_back(func); }
	void   set_next(OverloadSet *next) { m_next = next; }
	size_t count() const;
	OverloadSetIterator      begin();
	OverloadSetIterator      end();
	OverloadSetConstIterator begin() const;
	OverloadSetConstIterator end() const;
	OverloadSetConstIterator cbegin() const;
	OverloadSetConstIterator cend() const;

private:
	std::string dump_json() override;
};

class OverloadSetIterator
{
private:
	bool                           is_end = false;
	std::vector<IFunc *>::iterator m_iter;
	OverloadSet                   *m_curr_set;

public:
	OverloadSetIterator()
	    : is_end(true)
	{}
	OverloadSetIterator(std::vector<IFunc *>::iterator iter,
	                    OverloadSet                   *curr);
	OverloadSetIterator &operator++();
	bool    operator==(const OverloadSetIterator &iter) const;
	bool    operator!=(const OverloadSetIterator &iter) const;
	IFunc *&operator*() const { return *m_iter; }

private:
	void jump_until_valid();
};
class OverloadSetConstIterator
{
private:
	bool                                 is_end = false;
	std::vector<IFunc *>::const_iterator m_iter;
	const OverloadSet                   *m_curr_set;

public:
	OverloadSetConstIterator()
	    : is_end(true)
	{}
	OverloadSetConstIterator(
	    std::vector<IFunc *>::const_iterator iter,
	    const OverloadSet                   *curr);
	OverloadSetConstIterator &operator++();
	bool operator==(const OverloadSetConstIterator &iter) const;
	bool operator!=(const OverloadSetConstIterator &iter) const;
	IFunc *const &operator*() const { return *m_iter; }

private:
	void jump_until_valid();
};


class Env
{
public:
	Logger &logger;

private:
	Env                             *m_parent;
	std::vector<uptr<Env>>           m_subenvs;
	std::vector<uptr<IEntity>>       m_owned_entities;
	std::map<std::string, IEntity *> m_symbol_table;
	std::string                      m_scope_name;

private:
	explicit Env(Env *parent, Logger &logger)
	    : m_parent(parent)
	    , logger(logger)
	{}

public:
	static uptr<Env> create(std::string scope_name,
	                        Logger     &logger)

	{
		auto env          = make_uptr(new Env(nullptr, logger));
		env->m_scope_name = std::move(scope_name);
		return env;
	}

	static Env *create(Env *parent, Logger &logger)
	{
		auto env     = make_uptr(new Env(parent, logger));
		auto env_ptr = env.get();
		parent->m_subenvs.push_back(std::move(env));
		return env_ptr;
	}

	std::string get_qualifier() const
	{
		if (m_parent)
		{
			auto parent_qualifier = m_parent->get_qualifier();
			if (parent_qualifier.empty() == false)
				return parent_qualifier + "::" + m_scope_name;
			else
				return m_scope_name;
		}
		else
			return m_scope_name;
	}

	std::string get_full_qualified_name(
	    const std::string &unqualified) const
	{
		auto q = get_qualifier();
		if (!q.empty())
			return q + "::" + unqualified;
		return unqualified;
	}

	bool check_args(IFuncType                  *func,
	                const std::vector<IType *> &arg_types,
	                bool throw_error = false);

	void add(const std::string &name, IEntity *obj);
	void add(const std::string &name, uptr<IEntity> obj)
	{
		add(name, obj.get());
		m_owned_entities.push_back(std::move(obj));
	}

	/// 返回标识符对应的实体，存在子级隐藏父级名称的现象
	/// T必须是NamedEntity的子类，在找到名为ident的实体后会检查是不是T指定的类型。
	/// fixme: 不但要找到，还要找全
	template <typename T = IEntity>
	    requires std::derived_from<T, IEntity>
	T *get(const Ident &ident) const
	{
		std::string name = ident.name;
		if (m_symbol_table.contains(name))
		{
			// 检查它是不是T类型
			IEntity *ent = m_symbol_table.at(name);
			if constexpr (std::is_same_v<T, IEntity>)
			{
				return ent;
			}
			else
			{
				if (auto t = dynamic_cast<T *>(ent))
					return t;
				logger.log(ErrorUnexpectedToken(
				    ident.range.head, ident.range.tail));
				throw ExceptionPanic();
			}
		}
		// 一个都没有，问爹要
		if (m_parent)
			return m_parent->get<T>(ident);
		// 没有爹，哭
		logger.log(
		    ErrorUndefinedSymbol(ident.name, ident.range));
		throw ExceptionPanic();
	}

	IFunc *overload_resolution(
	    const Ident                &func_ident,
	    const std::vector<IType *> &arg_types);

	void add_built_in_facility();

	std::string dump_json();

	Env *get_parent() { return m_parent; }
	Env *get_root()
	{
		auto e = this;
		while (e->m_parent)
		{
			e = e->m_parent;
		}
		return e;
	}

private:
	OverloadSet *get_overload_set(const std::string &name)
	{
		if (m_symbol_table.contains(name))
		{
			if (auto set = dynamic_cast<OverloadSet *>(
			        m_symbol_table.at(name)))
			{
				return set;
			}
		}
		// 自己没有从父级找
		if (m_parent)
		{
			return m_parent->get_overload_set(name);
		}
		return nullptr;
	}
	void add_to_overload_set(OverloadSet       *overloads,
	                         IFunc             *func,
	                         const std::string &name)
};

struct EnvGuard
{
	Env *&curr_env;
	Env  *old_env;

	EnvGuard(Env *&curr_env, Env *new_env)
	    : curr_env(curr_env)
	{
		old_env  = curr_env;
		curr_env = new_env;
	}

	~EnvGuard() { curr_env = old_env; }
};

inline Env *create_env(Env *parent, Logger &logger)
{
	return Env::create(parent, logger);
}

} // namespace protolang