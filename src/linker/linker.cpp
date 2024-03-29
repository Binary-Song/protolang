#include "linker.h"
#include "linker/COFFLinker.h"
namespace protolang
{
std::unique_ptr<Linker> create_linker(LinkerType)
{
	return std::unique_ptr<Linker>(new COFFLinker());
}

Linker::Linker() = default;

} // namespace protolang