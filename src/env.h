#pragma once
#include <cassert>
#include <map>
#include <string>
#include <utility>
#include "logger.h"
#include "named_entity.h"
#include "token.h"
#include "typedef.h"
namespace protolang
{
class Env
{
public:
	explicit Env(Env *parent, Logger &logger)
	    : parent(parent)
	    , logger(logger)
	{}

	void add_non_func(uptr<NamedEntity> obj)
	{
		// 禁止用add添加函数，请用add_overload替代
		assert(dynamic_cast<NamedFunc *>(obj.get()) == nullptr);
		auto name = obj->ident.name;
		if (symbol_table.contains(name))
		{
			auto [begin, _] = symbol_table.equal_range(name);
			logger.log(ErrorSymbolRedefinition(
			    name, begin->second->ident.location, obj->ident.location));
			throw ExceptionPanic();
		}
		symbol_table.insert({obj->ident.name, obj.get()});
		entities.push_back(std::move(obj));
	}

	[[nodiscard]] void add_alias(const Ident &alias, const Ident &target_name)
	{
		// todo: 不许创建除了type-alias以外的alias。
		// 目前能为任何无歧义的名字创建别名
		NamedEntity *target = get_one(target_name);
		if (symbol_table.contains(alias.name))
		{
			auto [begin, _] = symbol_table.equal_range(alias.name);
			logger.log(ErrorSymbolRedefinition(
			    alias.name, begin->second->ident.location, alias.location));
			throw ExceptionPanic();
		}
		symbol_table.insert({alias.name, target});
	}

	[[nodiscard]] void add_func(uptr<NamedFunc> named_func)
	{
		// todo: 检查overload不能互相冲突，造成二义性调用

		// 函数名不能和变量名、类型名等冲突，但函数名可以重复
		if (symbol_table.contains(named_func->ident.name))
		{
			auto [val_begin, val_end] =
			    symbol_table.equal_range(named_func->ident.name);
			// 所有同名的玩意必须只能是函数
			for (auto iter = val_begin; iter != val_end; ++iter)
			{
				if (iter->second->named_entity_type() !=
				    NamedEntityType::NamedFunc)
				{
					logger.log(
					    ErrorSymbolRedefinition(named_func->ident.name,
					                            iter->second->ident.location,
					                            named_func->ident.location));
					throw ExceptionPanic();
				}
			}
			// ok: 有重名，但是函数
		}
		symbol_table.insert({named_func->ident.name, named_func.get()});
		entities.push_back(std::move(named_func)); // 不可早move!
	}

	/// 返回标识符对应的实体，存在子级隐藏父级名称的现象
	[[nodiscard]] NamedEntity *get_one(const Ident &ident) const
	{
		std::string name = ident.name;
		if (symbol_table.contains(name))
		{
			auto [begin, end] = symbol_table.equal_range(name);
			if (std::distance(begin, end) == 1)
			{
				// ok 正好找到1个
				return begin->second;
			}
			// 得到的是一堆函数重载
			logger.log(ErrorAmbiguousSymbol(ident.name, ident.location));
			throw ExceptionPanic();
		}
		// 一个都没有，问爹要
		if (parent)
		{
			return parent->get_one(ident);
		}
		// 没有爹，哭
		logger.log(ErrorUndefinedSymbol(ident.name, ident.location));
		throw ExceptionPanic();
	}

	/// 返回标识符对应的所有实体，包括子级和父级中所有同名实体
	[[nodiscard]] std::vector<NamedEntity *> get_all(const Ident &ident) const
	{
		std::vector<NamedEntity *> result;
		std::string                name = ident.name;
		auto [begin, end]               = symbol_table.equal_range(name);
		for (auto iter = begin; iter != end; ++iter)
		{
			result.push_back(iter->second);
		}
		// 如果有爹，加上爹的
		if (parent)
		{
			auto parents = parent->get_all(ident);
			result.insert(result.end(), parents.begin(), parents.end());
		}
		return result;
	}

	std::string dump_json() const
	{
		std::vector<NamedEntity *> vals;
		for (auto &&kv : symbol_table)
		{
			vals.push_back(kv.second);
		}
		if (parent)
			return std::format(R"({{ "this": {}, "parent": {} }})",
			                   dump_json_for_vector_of_ptr(vals),
			                   parent->dump_json());
		else
			return std::format(R"({{ "this": {} }})",
			                   dump_json_for_vector_of_ptr(vals));
	}

private:
	Env                                      *parent;
	Logger                                   &logger;
	std::vector<uptr<NamedEntity>>            entities;
	std::multimap<std::string, NamedEntity *> symbol_table;
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