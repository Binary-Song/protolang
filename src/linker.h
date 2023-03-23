#pragma once
#include <memory>
#include <string>
#include <vector>
namespace protolang
{

enum class LinkerType
{
	COFF
};

class Linker
{

public:
	explicit Linker();
	virtual ~Linker()                                  = default;
	virtual void link(const std::vector<std::string> &inputs,
	                  const std::string &output) const = 0;
};

std::unique_ptr<Linker> create_linker(LinkerType type);

} // namespace protolang
