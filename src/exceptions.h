#pragma once
#include <exception>
namespace protolang
{
class ExceptionLinkerNotFound : public std::exception
{
public:
	const char *what() const override
	{
		return "Linker cannot be found.";
	}
};
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

class ExceptionNotImplemented : public std::exception
{
public:
	const char *what() const override
	{
		return "Dev is too lazy to implement this functionality";
	}
};

class ExceptionCastError : public std::exception
{
public:
	const char *what() const override
	{
		return "Invalid under cast_implicit";
	}
};


} // namespace protolang