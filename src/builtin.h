#pragma once
#include <functional>
#include "entity_system.h"

namespace protolang
{
class Env;
void add_builtins(Env *);
} // namespace protolang