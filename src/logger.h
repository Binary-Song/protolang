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
	std::string comment;

	CodeRef() = default;
	CodeRef(const SrcRange &range, std::string comment = "")
	    : range(range)
	    , comment(std::move(comment))
	{}
};

class Logger;

struct Log : std::exception
{
	virtual ~Log() = default;
	virtual void print(Logger &logger) const;
};

struct Error : Log
{
	const char *what() const override
	{
		return "Error occurred.";
	}
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

	void print(const std::string &line) const
	{
		out << line << "\n";
	}
	void print(const std::string &comment,
	           const SrcRange    &range);
	void print(const CodeRef &ref);

	static int digits(int n)
	{
		if (n / 10 == 0)
			return 1;
		return 1 + digits(n / 10);
	}

	friend std::ostream &operator<<(Logger            &logger,
	                                const std::string &text)
	{
		return logger.out << text;
	}
};

} // namespace protolang