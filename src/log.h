#pragma once
#include <string>
#include <utility>
#include <vector>
#include "token.h"
namespace protolang
{
struct CodeRef
{
	Pos2D       first;
	Pos2D       last;
	std::string comment;

	CodeRef() = default;

	CodeRef(const Pos2D &first, const Pos2D &last, string comment = "")
	    : first(first)
	    , last(last)
	    , comment(std::move(comment))
	{}

	explicit CodeRef(const Token &token, string comment = "")
	    : CodeRef(token.first_pos, token.last_pos, std::move(comment))
	{}
};

class Log
{
public:
	enum class Level
	{
		Warning,
		Error,
		Fatal,
	};

public:
	std::vector<CodeRef> code_refs;

public:
	Log() = default;
	Log(const Pos2D &first, const Pos2D &last, std::string comment = "")
	{
		code_refs.push_back({first, last, comment});
	}
	Log(const Token &token, std::string comment = "")
	    : Log(token.first_pos, token.last_pos, std::move(comment))
	{}
	virtual ~Log()                                    = default;
	virtual void  desc_ascii(std::ostream &out) const = 0;
	virtual Level level() const                       = 0;
	virtual int   code() const                        = 0;
};

class ErrorAmbiguousInt : public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "The preceding `0` is redundant."
		       " Use the `0o` prefix if you want an octal literal.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1001; }
};

class ErrorUnknownChar : public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Unexpected character.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1002; }
};

class ErrorParenMismatch : public Log
{
public:
	bool left;
	ErrorParenMismatch(bool left, const Token &token)
	    : Log(token)
	    , left(left)
	{}
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Unmatched " << (left ? "left" : "right") << " parenthesis.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1003; }
};

class ErrorExpressionExpected : public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Expression expected.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1004; }
};

class ErrorUnexpectedToken:public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Unexpected token. ";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1005; }
};

class FatalFileError : public Log
{
public:
	std::string file_name;
	char        op;

	FatalFileError(std::string file_name, char operation)
	    : file_name(file_name)
	    , op(operation)
	{}

	virtual void desc_ascii(std::ostream &out) const override
	{
		if (op == 'r')
			out << "Cannot open file " << file_name;
		else if (op == 'w')
			out << "Cannot write into file " << file_name;
		else
			out << "Cannot access file " << file_name;
	}
	virtual Level level() const override { return Level::Fatal; }
	int           code() const override { return 9001; }
};

} // namespace protolang