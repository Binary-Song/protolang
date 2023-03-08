#pragma once
#include <string>
class Env;
namespace protolang
{

struct Type
{
	virtual std::string id(Env *env) const = 0;
};

struct IdentType : Type
{
	std::string name;

	std::string id(Env *env) const override;
};

} // namespace protolang