#include <iostream>
#include "source_code.h"
#include "encoding.h"
namespace protolang
{

static StringU8 read_line(std::istream &input)
{
	std::string line_str;
	std::getline(input,
	             line_str); // 你最好给我用utf8写源文件哦
	StringU8 line = as_u8(line_str);
	return line;
}

bool SourceCode::read(std::istream &input)
{
	auto line = read_line(input);
	while (input.good())
	{
		line += u8"\n";
		lines.push_back(line);
		str += line;
		line = read_line(input);
	}
	if (!input.eof())
		return false;
	lines.push_back(line);
	str += line;
	// 附送1行
	lines.emplace_back("\n");
	str += u8"\n";
	return true;
}
} // namespace protolang
