#pragma once
#include <cmath>
#include <iostream>
#include "exceptions.h"
#include "log.h"
#include "source_code.h"
namespace protolang
{
class Logger
{
	std::ostream &out;
	SourceCode   &src;

public:
	Logger(SourceCode &src, std::ostream &out)
	    : out(out)
	    , src(src)
	{}

	template <typename LogT>
	void log(LogT &&_error)
	{
		Log        &e = _error;
		std::string level_name;
		switch (e.level())
		{
		case Log::Level::Warning:
			level_name = "warning";
			break;
		case Log::Level::Error:
			level_name = "error";
			break;
		case Log::Level::Fatal:
			level_name = "fatal error";
			break;
		}

		out << level_name << " " << e.code() << ": ";
		e.desc_ascii(out);
		out << "\n";
		for (auto &&ref : e.code_refs)
		{
			print_code_ref(ref);
		}

		if (e.level() == Log::Level::Fatal)
			throw ExceptionFatalError();
	}

	void print(const Token &token)
	{
		out << "[" << token.first_pos.row << ":" << token.first_pos.column
		    << "~" << token.last_pos.row << ":" << token.last_pos.column
		    << "] T=" << (int)token.type << "\n";
		print_code_ref({token.first_pos, token.last_pos});
	}

	void print_code_ref(const CodeRef &ref)
	{
		// 输出第一行
		if (!ref.comment.empty())
			out << ref.comment << "\n";
		// 输出后续

		auto start = ref.first;
		auto end   = ref.last;

		std::string first_line   = src.lines[start.row];
		int         lineno_width = digits(end.row + 1);
		for (std::size_t i = start.row; i <= end.row; i++)
		{
			std::string _this_line = src.lines[i];
			std::string this_line(_this_line.begin(), _this_line.end() - 1);

			out << std::setw(lineno_width) << i + 1 << " | " << this_line
			    << "\n";

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

			out << std::string(underline_begin, ' ')
			    << std::string(underline_size, '^') << "\n";
		}
	}

	static int digits(int n)
	{
		if (n / 10 == 0)
			return 1;
		return 1 + digits(n / 10);
	}
};

} // namespace protolang