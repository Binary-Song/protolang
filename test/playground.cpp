#include <filesystem>
#include <fmt/xchar.h>
#include <iostream>
#include "encoding.h"
#include "encoding/win/encoding_win.h"
int main()
{
	using namespace protolang;

	StringU8     str   = "D:\\protolang-master";
	auto         wide1 = u82wide(str);
	std::wstring wide2 = L"D:\\protolang-master";

	auto s1    = wide1.size();
	auto s2    = wide2.size();
	auto path1 = std::filesystem::path{wide1};
	auto path2 = std::filesystem::path{wide2};

	try
	{
		auto a =
		    std::filesystem::directory_iterator(path1); // 抛异常
	}
	catch (...)
	{
		std::cout << "a fucked\n";
	}
	try
	{
		auto b =
		    std::filesystem::directory_iterator(path2); // 不抛
	}
	catch (...)
	{
		std::cout << "b fucked\n";
	}
}