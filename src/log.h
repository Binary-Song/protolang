#pragma once
#include <string>
#include <utility>
#include <vector>
#include "token.h"
namespace protolang
{

enum class LogCode
{
	ErrorAmbiguousInt = 1001,
	ErrorUnknownChar,
	ErrorParenMismatch,
	ErrorExpressionExpected,
	ErrorUnexpectedToken,
	ErrorSymbolRedefinition,
	ErrorUndefinedSymbol,
	ErrorAmbiguousSymbol,
	FatalFileError,
};

struct CodeRef
{
	Pos2D       first;
	Pos2D       last;
	std::string comment;

	CodeRef() = default;
	CodeRef(const Pos2DRange &range, std::string comment = "")
	    : first(range.first)
	    , last(range.last)
	    , comment(std::move(comment))
	{}
	CodeRef(const Pos2D &first, const Pos2D &last, std::string comment = "")
	    : first(first)
	    , last(last)
	    , comment(std::move(comment))
	{}

	explicit CodeRef(const Token &token, std::string comment = "")
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

class LogWithSymbol : public Log
{
public:
	std::string symbol;
	explicit LogWithSymbol(std::string symbol, const Pos2DRange &location)
	    : symbol(std::move(symbol))
	{
		code_refs.push_back({location.first, location.last, ""});
	}
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
	int code() const override { return (int)LogCode::ErrorAmbiguousInt; }
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
	int code() const override { return (int)LogCode::ErrorUnknownChar; }
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
	int code() const override { return (int)LogCode::ErrorParenMismatch; }
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
	int code() const override { return (int)LogCode::ErrorExpressionExpected; }
};

class ErrorUnexpectedToken : public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Unexpected token. ";
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorUnexpectedToken; }
};

class ErrorSymbolRedefinition : public Log
{
public:
	std::string symbol;
	ErrorSymbolRedefinition(std::string symbol,
	                        Pos2DRange  first,
	                        Pos2DRange  second)
	    : symbol(std::move(symbol))
	{
		this->code_refs.emplace_back(second, "redefined here");
		this->code_refs.emplace_back(first, "previously defined here");
	}

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Redefinition of symbol `" << symbol << "`.";
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorSymbolRedefinition; }
};

class ErrorUndefinedSymbol : public LogWithSymbol
{
public:
	using LogWithSymbol::LogWithSymbol;

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Undefined symbol `" << symbol << "`.";
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorUndefinedSymbol; }
};

class ErrorAmbiguousSymbol : public LogWithSymbol
{
public:
	std::string symbol;
	using LogWithSymbol::LogWithSymbol;

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Symbol `" << symbol << "` is ambiguous.";
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorAmbiguousSymbol; }
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
	int           code() const override { return (int)LogCode::FatalFileError; }
};

} // namespace protolang