#include "COFFLinker.h"

#include <Windows.h>
#include <codecvt>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include "encoding/win/encoding_win.h"
#include "guessing.h"

namespace protolang
{
std::filesystem::path COFFLinker::link(
    const std::vector<std::filesystem::path> &_inputs,
    const std::filesystem::path              &_output) const
{
	std::filesystem::path output{_output};
	output += ".exe";

	std::deque<StringU8> args = {
	    u8"/OUT:" + StringU8(output),
	    u8"/ENTRY:main",
	};
	for (auto &input : _inputs)
	{
		args.emplace_front(input);
	}

	auto     linker  = guess_linker_path();
	StringU8 command = u8'\"' + StringU8(linker) + u8"\" ";
	for (auto &arg : args)
	{
		command += u8'\"';
		command += arg;
		command += u8"\" ";
	}

	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
	std::wstring        wcommand = protolang::u8towide(command);

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
		return {};
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return output;
}

COFFLinker::COFFLinker() = default;
} // namespace protolang
