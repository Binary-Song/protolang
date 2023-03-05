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
		token = Token::make_keyword(text(), get_pos1(), get_pos2());
	}
	void rule_op() { token = Token::make_op(text(), get_pos1(), get_pos2()); }
	void rule_eof() { token = Token::make_eof(get_pos1()); };

private:
	Pos2D get_pos1() const { return {(u32)lineno() - 1, (u32)columno()}; }

	Pos2D get_pos2() const
	{
		return {(u32)lineno_end() - 1, (u32)columno_end()};
	}
};
} // namespace protolang