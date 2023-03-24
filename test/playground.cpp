#include <filesystem>
#include <fmt/xchar.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include "3rdparty/glob/glob.h"
#include "encoding.h"
#include "encoding/win/encoding_win.h"
int main()
{
	using namespace protolang;

	auto g = glob::glob(StringU8("C:\\Users\\yeh18\\Documents\\毕业").to_native());
	for(auto file : g)
	{
	}

//	StringU8 str = "C:\\Users\\yeh18\\Documents\\毕业\\1.txt";
//	auto     path1 = str.to_path();
//	std::error_code      ec;
//	llvm::raw_fd_ostream s(str.as_str(), ec);
//	auto data = StringU8("你好11111111111111111111").as_str();
//	s.write(data.data(), data.size());
//	s.flush();
//
//	StringU8 str2 = "C:\\Users\\yeh18\\Documents\\毕业";
//	std::filesystem::path path(str2.to_native());
}