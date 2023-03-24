#include <fmt/xchar.h>
#include <format>
#include "encoding.h"
#include <iostream>
int main()
{
	using namespace protolang;
	auto x = fmt::format(R"({{"obj":"IdentTypeExpr","ident":{}}})"_u8view,
	            ""_u8view);
}