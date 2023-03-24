#pragma once
#include <string_view>
#include <vector>
namespace protolang
{
int run_command(std::string_view              command,
                std::vector<std::string_view> args);
}