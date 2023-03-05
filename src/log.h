#pragma once
#include <string>
#include <vector>
#include "token.h"
namespace protolang
{
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
	Pos2D start;
	Pos2D end;
	Log(const Pos2D &start, const Pos2D &end)
	    : start(start)
	    , end(end)
	{}
	explicit Log(const Token &token)
	    : start(token.first_pos)
	    , end(token.last_pos)
	{}
	virtual ~Log()                              = default;
	virtual void  desc_ascii(std::ostream &out) = 0;
	virtual Level level() const                 = 0;
	virtual int   code() const                  = 0;
};

class ErrorAmbiguousInt : public Log
{
public:
	using Log::Log;
	virtual void desc_ascii(std::ostream &out) override
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
	virtual void desc_ascii(std::ostream &out) override
	{
		out << "Unexpected character.";
	}
	virtual Level level() const override { return Level::Error; }
	int           code() const override { return 1002; }
};

} // namespace protolang