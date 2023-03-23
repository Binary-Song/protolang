#pragma once
#include <iostream>
#include <string>
#include <vector>
namespace protolang
{
class SourceCode
{
public:
	explicit SourceCode() {}

	[[nodiscard]] bool read(std::istream &input)
	{
		u8str line;
		std::getline(input, line);
		while (input.good())
		{
			line += "\n";
			lines.push_back(line);
			str += line;
			std::getline(input, line);
		}
		if (!input.eof())
			return false;
		lines.push_back(line);
		str += line;
		// 附送1行
		lines.emplace_back("\n");
		str += "\n";
		return true;
	}

	u8str              str;
	std::vector<u8str> lines;
};
} // namespace protolang