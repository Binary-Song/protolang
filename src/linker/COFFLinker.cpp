#include "COFFLinker.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include "guessing.h"

#include <Windows.h>
#include <codecvt>
#include <string>

namespace protolang
{
void COFFLinker::link(const std::vector<u8str> &inputs,
                      const u8str              &output) const
{
	u8str             output_exe = output + u8".exe";
	std::deque<u8str> args       = {
        "/OUT:" + output_exe,
        "/ENTRY:main",
    };
	for (auto &input : inputs)
	{
		args.push_front(input);
	}

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
	std::wstring        wcommand;
	StringToWString(wcommand, command);
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
	WaitForSingleObject(pi.hProcess, INFINITE);
	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	std::cout << output_exe << std::endl;
}

COFFLinker::COFFLinker() = default;
} // namespace protolang
