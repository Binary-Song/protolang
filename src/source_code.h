#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "encoding.h"

namespace protolang
{
class SourceCode
{
public:
	explicit SourceCode() {}

	[[nodiscard]] bool read(std::istream &input);

	StringU8              str;
	std::vector<StringU8> lines;
};
} // namespace protolang