#pragma once
#include <map>
#include <string>
#include "types.h"
namespace protolang
{
struct Pos2D
{
	/// 行号（从0开始）
	u32 row    = 0;
	/// 列号（从0开始）
	u32 column = 0;
};

enum Keyword
{
	KW_VAR,
	KW_FUNC,
	KW_STRUCT,
	KW_CLASS,
	KW_RETURN,
	KW_IF,
	KW_ELSE,
	KW_WHILE,
	KW_TRUE,
	KW_FALSE,
};

static const std::map<std::string, Keyword> kw_map = {
    {   "var",    KW_VAR},
    {  "func",   KW_FUNC},
    {"struct", KW_STRUCT},
    { "class",  KW_CLASS},
    {"return", KW_RETURN},
    {    "if",     KW_IF},
    {  "else",   KW_ELSE},
    { "while",  KW_WHILE},
    {  "true",   KW_TRUE},
    { "false",  KW_FALSE},
};

struct Token
{
public:
	enum class Type
	{
		None,
		Int,
		Fp,
		Id,
		Keyword,
		Op,
		LeftParen,
		RightParen,
		SemiColumn,
		Str,
		Eof,
	};

public:
	Type        type = Type::None;
	u64         int_data;
	double      fp_data;
	std::string str_data;

	/// 第一个字符的位置
	Pos2D first_pos;
	/// 最后一个字符的位置
	Pos2D last_pos;

public:
	Token() = default;

	static Token make_int(u64 val, const Pos2D &firstPos, const Pos2D &lastPos)
	{
		return Token(Type::Int, firstPos, lastPos, val, 0);
	}

	static Token make_fp(double       val,
	                     const Pos2D &firstPos,
	                     const Pos2D &lastPos)
	{
		return Token(Type::Fp, firstPos, lastPos, 0, val);
	}

	static Token make_id(const std::string &str,
	                     const Pos2D       &firstPos,
	                     const Pos2D       &lastPos)
	{
		return Token(Type::Id, firstPos, lastPos, 0, 0, str);
	}

	static Token make_keyword(const std::string &str,
	                          const Pos2D       &firstPos,
	                          const Pos2D       &lastPos)
	{
		if (str == "var")
			return Token(Type::Keyword, firstPos, lastPos, 0, 0, str);
	}

	static Token make_op(const std::string &str,
	                     const Pos2D       &firstPos,
	                     const Pos2D       &lastPos)
	{
		return Token(Type::Op, firstPos, lastPos, 0, 0, str);
	}

	static Token make_paren(bool left, const Pos2D &pos)
	{
		return Token(left ? Type::LeftParen : Type::RightParen,
		             pos,
		             pos,
		             0,
		             0,
		             left ? "(" : ")");
	}

	static Token make_semicol(const Pos2D &pos)
	{
		return Token(Type::SemiColumn, pos, pos, 0, 0, ";");
	}

	static Token make_str(const std::string &str,
	                      const Pos2D       &firstPos,
	                      const Pos2D       &lastPos)
	{
		return Token(Type::Str, firstPos, lastPos, 0, 0, str);
	}

	static Token make_eof(const Pos2D &pos)
	{
		return Token(Type::Eof, pos, pos, 0, 0);
	}

private:
	Token(Type         type,
	      const Pos2D &firstPos,
	      const Pos2D &lastPos,
	      u64          intData,
	      double       fpData,
	      std::string  strData = "")
	    : type(type)
	    , first_pos(firstPos)
	    , last_pos(lastPos)
	    , int_data(intData)
	    , fp_data(fpData)
	    , str_data(std::move(strData))
	{}
};

} // namespace protolang