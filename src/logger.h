#pragma once
#include <exception>
#include <format>
#include "source_code.h"
#include "token.h"
namespace protolang
{
struct CodeRef
{
	SrcRange    range;
	u8str comment;

	CodeRef() = default;
	CodeRef(const SrcRange &range, u8str comment = "")
	    : range(range)
	    , comment(std::move(comment))
	{}
};

class Logger;

struct Error : std::exception
{
	virtual ~Error() = default;

	const char *what() const override
	{
		return "Error occurred.";
	}
	virtual void print(Logger &logger) const = 0;
};

class Logger
{
	std::ostream &out;
	SourceCode   &src;

public:
	Logger(SourceCode &src, std::ostream &out)
	    : out(out)
	    , src(src)
	{}

	void print(const u8str &line) const
	{
		out << line << "\n";
	}
	void print(const u8str &comment,
	           const SrcRange    &range);
	void print(const CodeRef &ref);

	static int digits(int n)
	{
		if (n / 10 == 0)
			return 1;
		return 1 + digits(n / 10);
	}

	friend std::ostream &operator<<(Logger            &logger,
	                                const u8str &text)
	{
		return logger.out << text << "\n";
	}
};

} // namespace protolang