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
	std::vector<uptr<IEntity>>            m_entities;
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

	void add_non_func(const std::string &name, uptr<IEntity> obj)
	{
		// 禁止用add添加函数，请用add_overload替代
		assert(dynamic_cast<IFunc *>(obj.get()) == nullptr);
		if (m_symbol_table.contains(name))
		{
			// 重定义了
			auto [begin, _] = m_symbol_table.equal_range(name);
			logger.log(ErrorSymbolRedefinition(
			    name,
			    begin->second->get_src_range(),
			    obj->get_src_range()));
			throw ExceptionPanic();
		}
		m_symbol_table.insert({name, obj.get()});
		m_entities.push_back(std::move(obj));
	}

	[[nodiscard]] void add_alias(const Ident &alias,
	                             const Ident &target_name)
	{
		// todo: 不许创建除了type-alias以外的alias。
		// 目前能为任何无歧义的名字创建别名
		IEntity *target = get_one(target_name);
		if (m_symbol_table.contains(alias.name))
		{
			auto [begin, _] =
			    m_symbol_table.equal_range(alias.name);
			logger.log(ErrorSymbolRedefinition(
			    alias.name,
			    begin->second->get_src_range(),
			    alias.range));
			throw ExceptionPanic();
		}
		m_symbol_table.insert({alias.name, target});
	}

	[[nodiscard]] void add_func(const std::string &name,
	                            uptr<IFunc>        func)
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
					    same_name_entity->get_src_range(),
					    func->get_src_range()));
					throw ExceptionPanic();
				}
			}
			// ok: 有重名，但是函数
		}
		m_symbol_table.insert({name, func.get()});
		m_entities.push_back(std::move(func)); // 不可早move!
	}

	/// 返回标识符对应的实体，存在子级隐藏父级名称的现象
	/// T必须是NamedEntity的子类，在找到名为ident的实体后会检查是不是T指定的类型。
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

	/// 返回标识符对应的所有实体，包括子级和父级中所有同名实体
	[[nodiscard]] std::vector<IEntity *> get_all(
	    const Ident &ident) const
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

	IFunc *overload_resolution(
	    const Ident                &func_ident,
	    const std::vector<const IType *> &arg_types);
	//
	//	void add_built_in_facility()
	//	{
	//		// 创建 int double
	//		add_builtin_type("Int");
	//		add_builtin_type("Double");
	//
	//		// 创建 加法
	//		add_builtin_op(
	//		    "+",
	//		    dynamic_cast<IdentTypeExpr
	//*>(builtin_exprs["Int"])); 		add_builtin_op("+",
	//		               dynamic_cast<IdentTypeExpr *>(
	//		                   builtin_exprs["Double"]));
	//	}
	//
	//	IType *get_builtin_type(const std::string &name,
	//	                        TypeChecker       *tc)
	//	{
	//		return builtin_exprs.at(name)->get_type(tc);
	//	}

	std::string dump_json() const
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
			return std::format(
			    R"({{ "this": {} }})",
			    dump_json_for_vector_of_ptr(vals));
	}

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

	//	void add_builtin_op(const std::string &op,
	//	                    const std::string &type);
	//
	//	void add_builtin_type(const std::string &name);
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