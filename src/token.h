#pragma once
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
		return Token(Type::Keyword, firstPos, lastPos, 0, 0, str);
	}

	static Token make_op(const std::string &str,
	                     const Pos2D       &firstPos,
	                     const Pos2D       &lastPos)
	{
		return Token(Type::Op, firstPos, lastPos, 0, 0, str);
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
	{}
};

} // namespace protolang