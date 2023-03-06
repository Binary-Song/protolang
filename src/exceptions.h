#pragma once
#include <exception>
namespace protolang
{
class ExceptionFatalError : public std::exception
{
public:
	const char *what() const override
	{
		return "A fatal error occurred during compilation.";
	}
};

class ExceptionPanic : public std::exception
{
public:
	const char *what() const override { return "Panic mode on"; }
};
} // namespace protolang