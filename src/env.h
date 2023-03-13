#pragma once
#include <cassert>
#include <concepts>
#include <map>
#include <string>
#include <utility>
#include "entity_system.h"
#include "logger.h"
#include "token.h"
#include "typedef.h"
#include "util.h"
namespace protolang
{
class Env
{
public:
	Logger &logger;

private:
	Env                                  *m_parent;
	std::vector<uptr<Env>>                m_subenvs;
	std::vector<uptr<IEntity>>            m_owned_entities;
	std::vector<IEntity *>                m_entities;
	std::multimap<std::string, IEntity *> m_symbol_table;

private:
	explicit Env(Env *parent, Logger &logger)
	    : m_parent(parent)
	    , logger(logger)
	{}

public:
	static uptr<Env> create(Logger &logger)
	{
		auto env = make_uptr(new Env(nullptr, logger));
		return env;
	}

	static Env *create(Env *parent, Logger &logger)
	{
		auto env     = make_uptr(new Env(parent, logger));
		auto env_ptr = env.get();
		parent->m_subenvs.push_back(std::move(env));
		return env_ptr;
	}

	bool check_args(const IFuncType                  *func,
	                const std::vector<const IType *> &arg_types,
	                bool throw_error = false);

	void add_non_func(const std::string &name, IEntity *obj);
	void add_non_func(const std::string &name, uptr<IEntity> obj)
	{
		add_non_func(name, obj.get());
		m_owned_entities.push_back(std::move(obj));
	}

	void add_alias(const Ident &alias, const Ident &target_name);

	void add_func(const std::string &name, IFunc *func);
	void add_func(const std::string &name, uptr<IFunc> obj)
	{
		add_func(name, obj.get());
		m_owned_entities.push_back(std::move(obj));
	}
	/// 返回标识符对应的实体，存在子级隐藏父级名称的现象
	/// T必须是NamedEntity的子类，在找到名为ident的实体后会检查是不是T指定的类型。
	/// fixme: 不但要找到，还要找全
	template <typename T = IEntity>
	    requires std::derived_from<T, IEntity>
	T *get_one(const Ident &ident) const
	{
		std::string name = ident.name;
		if (m_symbol_table.contains(name))
		{
			auto [begin, end] = m_symbol_table.equal_range(name);
			if (std::distance(begin, end) == 1)
			{
				// ok 正好找到1个
				// 检查它是不是T类型
				IEntity *ent = begin->second;
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
			// 得到的是一堆函数重载
			logger.log(
			    ErrorAmbiguousSymbol(ident.name, ident.range));
			throw ExceptionPanic();
		}
		// 一个都没有，问爹要
		if (m_parent)
		{
			return m_parent->get_one<T>(ident);
		}
		// 没有爹，哭
		logger.log(
		    ErrorUndefinedSymbol(ident.name, ident.range)); 
		throw ExceptionPanic();
	}

	/// 返回标识符对应的所有实体，包括子级和父级中所有同名函数
	[[nodiscard]] std::vector<IFunc *> get_overloads(
	    const Ident &ident) const;

	[[nodiscard]] std::vector<IEntity*> search();

	IFunc *overload_resolution(
	    const Ident                      &func_ident,
	    const std::vector<const IType *> &arg_types);

	void add_built_in_facility();

	std::string dump_json() const;

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

} // namespace protolang