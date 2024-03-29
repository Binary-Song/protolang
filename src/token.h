#pragma once
#include <cassert>
#include <map>
#include <string>
#include <utility>
#include "encoding.h"
#include "typedef.h"

namespace protolang
{
struct SrcPos
{
	/// 行号（从0开始）
	u32 row    = 0;
	/// 列号（从0开始）
	u32 column = 0;

	bool operator==(const SrcPos &rhs) const
	{
		return row == rhs.row && column == rhs.column;
	}
	bool operator!=(const SrcPos &rhs) const
	{
		return !(rhs == *this);
	}
};
struct SrcRange
{
	SrcPos head;
	SrcPos tail;

	bool operator==(const SrcRange &rhs) const
	{
		return head == rhs.head && tail == rhs.tail;
	}
	bool operator!=(const SrcRange &rhs) const
	{
		return !(rhs == *this);
	}
};
static i64 operator<=>(const SrcPos &a, const SrcPos &b)
{
	auto row_diff = (i64)a.row - (i64)b.row;
	return row_diff == 0 ? (i64)a.column - (i64)b.column
	                     : row_diff;
}

// 默认 head < tail m
static SrcRange range_union(const SrcRange &first,
                            const SrcRange &second)
{
	assert(first.head <= first.tail);
	assert(second.head <= second.tail);
	return SrcRange{std::min(first.head, second.head),
	                std::max(first.tail, second.tail)};
}

static inline SrcRange operator+(const SrcRange &first,
                                 const SrcRange &second)
{
	return range_union(first, second);
}

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
	KW_AS,
	KW_IS,
};

static const std::map<StringU8, Keyword> kw_map = {
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
    {    "as",     KW_AS},
    {    "is",     KW_IS},
};

inline StringU8 kw_map_rev(Keyword kw)
{
	for (auto &&kv : kw_map)
	{
		if (kv.second == kw)
		{
			return kv.first;
		}
	}
	return "";
}

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
		LeftBrace,
		RightBrace,
		LeftBracket,
		RightBracket,
		SemiColumn,
		Column,
		Comma,
		Arrow,
		Str,
		Eof,
	};

public:
	Type     type = Type::None;
	u64      int_data;
	double   fp_data;
	StringU8 str_data;

	/// 第一个字符的位置
	SrcPos first_pos;
	/// 最后一个字符的位置
	SrcPos last_pos;

public:
	Token() = default;
	Token(Type          type,
	      const SrcPos &firstPos,
	      const SrcPos &lastPos,
	      u64           intData,
	      double        fpData,
	      StringU8      strData = "")
	    : type(type)
	    , first_pos(firstPos)
	    , last_pos(lastPos)
	    , int_data(intData)
	    , fp_data(fpData)
	    , str_data(std::move(strData))
	{}
	SrcRange range() const { return {first_pos, last_pos}; }

	static Token make_int(u64           val,
	                      const SrcPos &firstPos,
	                      const SrcPos &lastPos)
	{
		return Token(Type::Int, firstPos, lastPos, val, 0);
	}

	static Token make_fp(double        val,
	                     const SrcPos &firstPos,
	                     const SrcPos &lastPos)
	{
		return Token(Type::Fp, firstPos, lastPos, 0, val);
	}

	static Token make_id(const StringU8 &str,
	                     const SrcPos   &firstPos,
	                     const SrcPos   &lastPos)
	{
		return Token(Type::Id, firstPos, lastPos, 0, 0, str);
	}

	static Token make_keyword(const StringU8 &str,
	                          const SrcPos   &firstPos,
	                          const SrcPos   &lastPos)
	{
		return Token(Type::Keyword,
		             firstPos,
		             lastPos,
		             kw_map.at(str),
		             0,
		             str);
	}

	static Token make_op(const StringU8 &str,
	                     const SrcPos   &firstPos,
	                     const SrcPos   &lastPos)
	{
		return Token(Type::Op, firstPos, lastPos, 0, 0, str);
	}

	static Token make_paren(bool left, const SrcPos &pos)
	{
		return Token(left ? Type::LeftParen : Type::RightParen,
		             pos,
		             pos,
		             0,
		             0,
		             left ? "(" : ")");
	}

	static Token make_len1(Type          type,
	                       const SrcPos &pos,
	                       const char   *literal)
	{
		return Token(type, pos, pos, 0, 0, as_u8(literal));
	}

	static Token make_str(const StringU8 &str,
	                      const SrcPos   &firstPos,
	                      const SrcPos   &lastPos)
	{
		return Token(Type::Str, firstPos, lastPos, 0, 0, str);
	}

	static Token make_eof(const SrcPos &pos)
	{
		return Token(Type::Eof, pos, pos, 0, 0);
	}
};

} // namespace protolang