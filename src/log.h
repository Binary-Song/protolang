#pragma once
#include <format>
#include <string>
#include <utility>
#include <vector>
#include "token.h"
namespace protolang
{

enum class LogCode
{
	/// ERROR

	ErrorAmbiguousInt = 1001,
	ErrorUnknownChar,
	ErrorParenMismatch,
	ErrorExpressionExpected,
	ErrorUnexpectedToken,
	ErrorSymbolRedefinition,
	ErrorUndefinedSymbol,
	ErrorAmbiguousSymbol,
	ErrorSymbolIsNotAType,
	ErrorNoMatchingOverload,
	ErrorMultipleMatchingOverload,
	ErrorTypeMismatch,

	/// FATAL

	FatalFileError = 4001,
};

struct CodeRef
{
	SrcPos first = {static_cast<u32>(-1), static_cast<u32>(-1)};
	SrcPos last  = {static_cast<u32>(-1), static_cast<u32>(-1)};
	std::string comment;

	CodeRef() = default;
	CodeRef(const SrcRange &range, std::string comment = "")
	    : first(range.head)
	    , last(range.tail)
	    , comment(std::move(comment))
	{}
	CodeRef(const SrcPos &first, const SrcPos &last, std::string comment = "")
	    : first(first)
	    , last(last)
	    , comment(std::move(comment))
	{}

	explicit CodeRef(const Token &token, std::string comment = "")
	    : CodeRef(token.first_pos, token.last_pos, std::move(comment))
	{}

	bool operator==(const CodeRef &rhs) const
	{
		return first == rhs.first && last == rhs.last && comment == rhs.comment;
	}
	bool operator!=(const CodeRef &rhs) const { return !(rhs == *this); }
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
	Log(const SrcPos &first, const SrcPos &last, std::string comment = "")
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
	explicit LogWithSymbol(Ident ident)
	    : LogWithSymbol(ident.name, ident.range)
	{}
	explicit LogWithSymbol(std::string symbol, const SrcRange &location)
	    : symbol(std::move(symbol))
	{
		code_refs.push_back({location.head, location.tail, ""});
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
	                        SrcRange    first,
	                        SrcRange    second)
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

class ErrorSymbolKindIncorrect : public LogWithSymbol
{
public:
	std::string expected;
	std::string got;

	explicit ErrorSymbolKindIncorrect(Ident       ident,
	                                  std::string expected,
	                                  std::string got)
	    : LogWithSymbol(ident.name, ident.range)
	    , expected(expected)
	    , got(got)
	{}

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << std::format("Expected {}, got {}.", expected, got);
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorSymbolIsNotAType; }
};

class ErrorNoMatchingOverload : public LogWithSymbol
{
	std::string symbol;
	using LogWithSymbol::LogWithSymbol;

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Arguments cannot fit in parameters. ";
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorNoMatchingOverload; }
};

class ErrorMultipleMatchingOverload : public LogWithSymbol
{
	std::string symbol;
	using LogWithSymbol::LogWithSymbol;

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << "Multiple matching overloads.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override
	{
		return (int)LogCode::ErrorMultipleMatchingOverload;
	}
};

class ErrorTypeMismatch : public Log
{
public:
	std::string param_type, arg_type;
	ErrorTypeMismatch(std::string arg_type,
	                  SrcRange    arg_range,
	                  std::string param_type,
	                  SrcRange    param_range)
	    : param_type(param_type)
	    , arg_type(arg_type)
	{
		code_refs.push_back(CodeRef{arg_range, //
		                            std::format("actual type `{}`", arg_type)});
		code_refs.push_back(
		    CodeRef{param_range, //
		            std::format("expected type `{}`", param_type)});
	}

	virtual void desc_ascii(std::ostream &out) const override
	{
		out << std::format(
		    "Type mismatch. Cannot fit a `{}` into `{}`", arg_type, param_type);
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorTypeMismatch; }
};

class ErrorNotCallable : public Log
{
public:
	std::string  actual_type;
	virtual void desc_ascii(std::ostream &out) const override
	{
		out << std::format("Trying to call on type `{}` which is not callable",
		                   actual_type);
	}
	virtual Level level() const override { return Level::Error; }
	int code() const override { return (int)LogCode::ErrorTypeMismatch; }
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