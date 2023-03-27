#pragma once
#include <cassert>
#include <concepts>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "builtin.h"
#include "entity_system.h"
#include "log.h"
#include "logger.h"
#include "overloadset.h"
#include "token.h"
#include "typedef.h"
#include "util.h"
namespace protolang
{

template <bool do_throw = false>
bool check_forward_ref(const Ident &ref, IEntity *ent);

class Env
{
public:
	Logger &logger;

private:
	Env                          *m_parent;
	std::vector<uptr<Env>>        m_subenvs;
	std::vector<uptr<IEntity>>    m_owned_entities;
	std::map<StringU8, IEntity *> m_symbol_table;
	std::map<StringU8, IEntity *> m_keyword_symbol_table;
	StringU8                      m_scope_name;

private:
	explicit Env(Env *parent, Logger &logger)
	    : m_parent(parent)
	    , logger(logger)
	{}

public:
	static uptr<Env> create_root(Logger &logger)

	{
		auto env          = make_uptr(new Env(nullptr, logger));
		env->m_scope_name = u8"";
		return env;
	}

	static Env *create(Env *parent, Logger &logger)
	{
		auto env     = make_uptr(new Env(parent, logger));
		auto env_ptr = env.get();
		parent->m_subenvs.push_back(std::move(env));
		return env_ptr;
	}

	const Env *get_root() const
	{
		auto e = this;
		while (e->m_parent)
		{
			e = e->m_parent;
		}
		return e;
	}

	StringU8 get_qualifier() const
	{
		if (m_parent)
		{
			auto parent_qualifier = m_parent->get_qualifier();
			if (parent_qualifier.empty() == false)
				return parent_qualifier + u8"::" + m_scope_name;
			else
				return m_scope_name;
		}
		else
			return m_scope_name;
	}

	StringU8 get_full_qualified_name(
	    const StringU8 &unqualified) const
	{
		auto q = get_qualifier();
		if (!q.empty())
			return q + u8"::" + unqualified;
		return unqualified;
	}

	static bool check_args(IFuncType                  *func,
	                       const std::vector<IType *> &arg_types,
	                       bool throw_error,
	                       bool strict);

	void add(const Ident &name, IEntity *obj)
	{
		add_to(name, obj, this->m_symbol_table);
	}
	void add(const Ident &name, uptr<IEntity> obj)
	{
		add(name, obj.get());
		m_owned_entities.push_back(std::move(obj));
	}
	void add_keyword(const StringU8 &kw, IEntity *obj)
	{
		get_root()->add_to(Ident{kw, SrcRange{}},
		                   obj,
		                   this->m_keyword_symbol_table);
	}
	void add_keyword(const StringU8 &kw, uptr<IEntity> obj)
	{
		add_keyword(kw, obj.get());
		m_owned_entities.push_back(std::move(obj));
	}

	/// 返回标识符对应的实体，存在子级隐藏父级名称的现象
	/// T必须是NamedEntity的子类，在找到名为ident的实体后会检查是不是T指定的类型。
	template <std::derived_from<IEntity> T           = IEntity,
	          bool                       forward_ref = true,
	          bool                       look_at_kw_table = true>
	T *get(const Ident &ident) const;

	template <std::derived_from<IEntity> T = IEntity>
	T *get_backwards(const Ident &ident) const
	{
		return get<T, false>(ident);
	}

	IEntity *get_keyword_entity(const StringU8 &keyword) const
	{
		auto &&symb_tbl = get_root()->m_keyword_symbol_table;
		if (symb_tbl.contains(keyword))
		{
			return symb_tbl.at(keyword);
		}
		return nullptr;
	}

	template <std::derived_from<IEntity> T>
	T *get_keyword_entity(const StringU8 &keyword) const
	{
		auto entity =
		    dyn_cast_force<T *>(get_keyword_entity(keyword));
		assert(entity);
		return entity;
	}

	IType *get_void() const
	{
		return dyn_cast_force<IType *>(
		    get_keyword_entity("void"));
	}

	IType *get_bool() const
	{
		return dyn_cast_force<IType *>(
		    get_keyword_entity("bool"));
	}

	IOp *overload_resolution(
	    const Ident                &func_ident,
	    const std::vector<IType *> &arg_types);

	void add_built_in_facility();

	StringU8 dump_json();

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
	OverloadSet *get_overload_set(const StringU8 &name)
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
	void add_to_overload_set(OverloadSet    *overloads,
	                         IOp            *func,
	                         const StringU8 &name);

	void add_to(const Ident                   &name,
	            IEntity                       *entity,
	            std::map<StringU8, IEntity *> &to);
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