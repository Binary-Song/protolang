#pragma once
#include <sstream>
#include "lex.yy.h"
#include "logger.h"
#include "source_code.h"
#include "token.h"
namespace protolang
{
class Lexer : private protolang_generated::Lexer
{
public:
	Lexer(SourceCode &code, Logger &logger)
	    : logger(logger)
	    , code(code)
	{
		this->in(code.str); // 基类方法：设置输入源
	}
	SourceCode &code;
	Logger     &logger;
	Token       token;

	std::vector<Token> scan()
	{
		std::vector<Token> tokens;
		int                ret;
		while ((ret = lex()) == 0)
		{
			tokens.push_back(token);
		}
		if (ret == -1)
		{
			tokens.push_back(token);
			return tokens;
		}
		else
			return {};
	}

	using protolang_generated::Lexer::lex;

protected:
	int vlex(Rule ruleno) override
	{
		switch (ruleno)
		{
		case Rule::int_dec:
			rule_int(10, 0);
			break;
		case Rule::int_oct:
			rule_int(8, 2);
			break;
		case Rule::int_hex:
			rule_int(16, 2);
			break;
		case Rule::int_bin:
			rule_int(2, 2);
			break;
		case Rule::fp:
			rule_fp();
			break;
		case Rule::id:
			rule_id();
			break;
		case Rule::keyword:
			rule_keyword();
			break;
		case Rule::op:
			rule_op();
			break;
		case Rule::left_paren:
			rule_paren(true);
			break;
		case Rule::right_paren:
			rule_paren(false);
			break;
		case Rule::semicol:
			rule_semicol();
			break;
		case Rule::col:
			rule_col();
			break;
		case Rule::comma:
			rule_comma();
			break;
		case Rule::arrow:
			rule_arrow();
			break;
		case Rule::left_brace:
			rule_left_brace();
			break;
		case Rule::right_brace:
			rule_right_brace();
			break;
		case Rule::left_bracket:
			rule_left_bracket();
			break;
		case Rule::right_bracket:
			rule_right_bracket();
			break;
		case Rule::str:
			break;
		case Rule::eof:
			rule_eof();
			return -1;
		case Rule::err_amb_int:
			logger.log(ErrorAmbiguousInt(get_pos1(), get_pos2()));
			return 1;
		case Rule::err_unknown_char:
			logger.log(ErrorUnknownChar(get_pos1(), get_pos2()));
			return 1;
		default:
			return 1;
		}
		return 0;
	}

	void rule_int(int base, int begin_index)
	{
		auto [text, text_len] = matcher()[0];
		u64 val               = 0;
		for (std::size_t i = begin_index; i < text_len; i++)
		{
			char ch = text[i];
			val *= base;
			if (ch <= '9')
				val += text[i] - '0';
			else if (ch <= 'F') // 'a' == 97  'A' == 65  '9' == 57
				val += text[i] - 'A' + 10;
			else
				val += text[i] - 'a' + 10;
		}
		token = Token::make_int(val, get_pos1(), get_pos2());
	}

	void rule_fp()
	{
		auto [text, text_len] = matcher()[0];

		double      val = 0;
		std::size_t i   = 0;
		// 整数部分
		for (; i < text_len; i++)
		{
			char ch = text[i];
			if (ch == '.')
				break;
			val *= 10;
			val += ch - '0';
		}
		// 小数部分
		double multiplier = 0.1;
		for (; i < text_len; i++)
		{
			char ch = text[i];
			val += (ch - '0') * multiplier;
			multiplier /= 10;
		}
		token = Token::make_fp(val, get_pos1(), get_pos2());
	}

	void rule_id() { token = Token::make_id(text(), get_pos1(), get_pos2()); }
	void rule_keyword()
	{
		token          = Token::make_keyword(text(), get_pos1(), get_pos2());
		token.int_data = kw_map.at(text());
	}
	void rule_op() { token = Token::make_op(text(), get_pos1(), get_pos2()); }
	void rule_eof() { token = Token::make_eof(get_pos1()); };
	void rule_paren(bool left) { token = Token::make_paren(left, get_pos1()); }
	void rule_semicol()
	{
		token = Token::make_len1(Token::Type::SemiColumn, get_pos1(), ";");
	}
	void rule_comma()
	{
		token = Token::make_len1(Token::Type::Comma, get_pos1(), ",");
	}
	void rule_col()
	{
		token = Token::make_len1(Token::Type::Column, get_pos1(), ":");
	}
	void rule_arrow()
	{
		token = Token(Token::Type::Arrow, get_pos1(), get_pos2(), 0, 0, "->");
	}
	void rule_left_brace()
	{
		token =
		    Token(Token::Type::LeftBrace, get_pos1(), get_pos2(), 0, 0, "{");
	}
	void rule_right_brace()
	{
		token =
		    Token(Token::Type::RightBrace, get_pos1(), get_pos2(), 0, 0, "}");
	}
	void rule_left_bracket()
	{
		token =
		    Token(Token::Type::LeftBracket, get_pos1(), get_pos2(), 0, 0, "[");
	}
	void rule_right_bracket()
	{
		token =
		    Token(Token::Type::RightBracket, get_pos1(), get_pos2(), 0, 0, "]");
	}

private:
	SrcPos get_pos1() const { return {(u32)lineno() - 1, (u32)columno()}; }

	SrcPos get_pos2() const
	{
		return {(u32)lineno_end() - 1, (u32)columno_end()};
	}
};
} // namespace protolang