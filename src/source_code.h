#pragma once
#include <iostream>
namespace protolang
{
class SourceCode
{
public:
	explicit SourceCode(std::istream &input)
	{
		std::string line;
		std::getline(input, line);
		while (input.good())
		{
			line += "\n";
			lines.push_back(line);
			str += line;
			std::getline(input, line);
		}
		lines.push_back(line);
		str += line;
		// 附送1行
		lines.emplace_back("\n");
		str += "\n";
	}

	std::string              str;
	std::vector<std::string> lines;
};
} // namespace protolang