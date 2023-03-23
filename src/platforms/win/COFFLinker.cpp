//
// Created by wps on 2023/3/23.
//

#include "COFFLinker.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include <lld/Common/Driver.h>
#include "guessing.h"

#include <Windows.h>
#include <codecvt>
#include <string>
int StringToWString(std::wstring &ws, const std::string &s)
{
	std::wstring wsTmp(s.begin(), s.end());

	ws = wsTmp;

	return 0;
}


namespace protolang
{
void COFFLinker::link(const std::vector<std::string> &inputs,
                      const std::string &output) const
{
	std::string             output_exe = output + ".exe";
	std::deque<std::string> args       = {
        "/OUT:" + output_exe,
        "/ENTRY:main",
    };
	for (auto &input : inputs)
	{
		args.push_front(input);
	}

	std::string              str_out;
	std::string              str_err;
	llvm::raw_string_ostream out(str_out);
	llvm::raw_string_ostream err(str_err);

	auto linker  = guess_linker_path();
	auto command = '\"' + linker + "\" ";
	for (auto &arg : args)
	{
		command += '\"';
		command += arg;
		command += "\" ";
	}

	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
	std::wstring wcommand;
	StringToWString(wcommand,command);
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!::CreateProcess(NULL,
	                     wcommand.data(),
	                     NULL,
	                     NULL,
	                     FALSE,
	                     0,
	                     NULL,
	                     NULL,
	                     &si,
	                     &pi))
	{
		return;
	}
	std::cout << output_exe << std::endl;
}

COFFLinker::COFFLinker() = default;
} // namespace protolang