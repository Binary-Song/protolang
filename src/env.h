#pragma once
#include <map>
#include <string>
#include <utility>
#include "named_entity.h"
#include "typedef.h"
namespace protolang
{
class Env
{
public:
	explicit Env(Env *parent)
	    : parent(parent)
	{}

	[[nodiscard]] bool add(const std::string &name, uptr<NamedEntity> obj)
	{
		if (symbol_table.contains(name))
		{
			return false;
		}
		entities.push_back(std::move(obj));
		symbol_table[name] = obj.get();
		return true;
	}

	[[nodiscard]] bool add_alias(const std::string &alias,
	                             const std::string &to)
	{
		// todo: 不许创建除了type-alias以外的alias。
		NamedEntity *target = get(to);
		if (symbol_table.contains(alias) || target == nullptr)
			return false;
		symbol_table[alias] = target;
		return true;
	}

	NamedEntity *get(const std::string &name) const
	{
		if (symbol_table.contains(name))
		{
			return symbol_table.at(name);
		}
		return parent->get(name);
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
	Env                                 *parent;
	std::vector<uptr<NamedEntity>>       entities;
	std::map<std::string, NamedEntity *> symbol_table;
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