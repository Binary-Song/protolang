#include <cmath>
#include <iomanip>
#include <iostream>
#include "logger.h"
#include "exceptions.h"
#include "ident.h"
#include "log.h"
namespace protolang
{

void Logger::print(const CodeRef &ref)
{

	// 输出第一行
	if (!ref.comment.empty())
		out << ref.comment.to_native() << "\n";

	if (ref.range == SrcRange{})
	{
		out << "[No Source]\n";
		return;
	}

	// 输出后续

	auto start = ref.range.head;
	auto end   = ref.range.tail;

	StringU8 first_line   = src.lines[start.row];
	int      lineno_width = digits(end.row + 1);
	for (std::size_t i = start.row; i <= end.row; i++)
	{
		StringU8 _this_line = src.lines[i];
		StringU8 this_line(_this_line.begin(),
		                   _this_line.end() - 1);

		out << std::setw(lineno_width) << i + 1 << " | "
		    << this_line.to_native() << "\n";

		// 决定当前行下划线的位置和长度
		std::size_t underline_begin = lineno_width + 3;
		std::size_t underline_size  = 0;
		if (start.row == end.row) // 只有1行
		{
			underline_begin += start.column;
			underline_size = end.column - start.column + 1;
		}
		else if (i == start.row) // 跨越多行时，第一行
		{
			underline_begin += start.column;
			underline_size = this_line.size() - start.column;
		}
		else if (i == end.row)
		{
			underline_size = end.column + 1;
		}
		else
		{
			underline_size = this_line.size();
		}

		out << StringU8(underline_begin, ' ').to_native()
		    << StringU8(underline_size, '^').to_native() << "\n";
	}
}
void Logger::print(const StringU8 &comment,
                   const SrcRange &range)
{
	print(CodeRef{range, comment});
}
} // namespace protolang