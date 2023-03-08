#pragma once
#include <map>
#include <string>
#include <utility>
#include "namedobject.h"
#include "typedef.h"
namespace protolang
{
class Env
{
public:
	explicit Env(Env *parent)
	    : parent(parent)
	{}

	[[nodiscard]] bool add(const std::string &name, uptr<NamedObject> obj)
	{
		if (symbol_table.contains(name))
		{
			return false;
		}
		symbol_table[name] = std::move(obj);
		return true;
	}

	NamedObject *get(const std::string &name) const
	{
		if (symbol_table.contains(name))
		{
			return symbol_table.at(name).get();
		}
		return parent->get(name);
	}
	std::string dump_json() const
	{
		std::vector<NamedObject *> vals;
		for (auto &&kv : symbol_table)
		{
			vals.push_back(kv.second.get());
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
	Env                                     *parent;
	std::map<std::string, uptr<NamedObject>> symbol_table;
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