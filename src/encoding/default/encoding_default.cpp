#pragma once
#include "encoding/encoding.h"
namespace protolang
{

std::string to_narrow_def(const u8str &)
{
	return std::string();
}
u8str to_u8str_def(const std::string &)
{
	return protolang::u8str();
}

}