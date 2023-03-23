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
	if (ref.range == SrcRange{})
		return;

	// 输出第一行
	if (!ref.comment.empty())
		out << ref.comment << "\n";
	// 输出后续

	auto start = ref.range.head;
	auto end   = ref.range.tail;

	u8str first_line   = src.lines[start.row];
	int         lineno_width = digits(end.row + 1);
	for (std::size_t i = start.row; i <= end.row; i++)
	{
		u8str _this_line = src.lines[i];
		u8str this_line(_this_line.begin(),
		                      _this_line.end() - 1);

		out << std::setw(lineno_width) << i + 1 << " | "
		    << this_line << "\n";

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

		out << u8str(underline_begin, ' ')
		    << u8str(underline_size, '^') << "\n";
	}
}
void Logger::print(const u8str &comment,
                   const SrcRange    &range)
{
	print(CodeRef{range, comment});
}
} // namespace protolang