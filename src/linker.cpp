#include <cstdlib>
#include <deque>
#include <iostream>
#include <lld/Common/Driver.h>
#include <utility>
#include "linker.h"
#include "guessing.h"
#include "platforms/win/COFFLinker.h"
namespace protolang
{
std::unique_ptr<Linker> create_linker(LinkerType)
{
	return std::unique_ptr<Linker>(new COFFLinker());
}

Linker::Linker() = default;

} // namespace protolang