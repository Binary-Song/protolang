#include "type.h"
#include "env.h"
namespace protolang
{
std::string IdentType::id(Env *env) const
{
	env->get(name)
}

} // namespace protolang