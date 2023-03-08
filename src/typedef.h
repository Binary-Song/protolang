#pragma once
#include <cctype>
#include <memory>
#include <format>
namespace protolang
{

using i32 = std::int32_t;
using u32 = std::uint32_t;

using i64 = std::int64_t;
using u64 = std::uint64_t;

template <typename T>
using uptr = std::unique_ptr<T>;

template <typename T>
inline std::string dump_json_for_vector_of_ptr(const std::vector<T> &data);
template <typename T>
inline std::string dump_json_for_vector(const std::vector<T> &data);
template <typename T>
inline std::string dump_json_for_basic_vector(const std::vector<T> &data)
{
	std::string json = "[";
	for (auto &&item : data)
	{
		json += std::format("{}", item);
		json += ",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += "]";
	return json;
}
template <typename T>
std::string dump_json_for_vector_of_ptr(const vector<T> &data)
{
	std::string json = "[";
	for (auto &&item : data)
	{
		json += item->dump_json();
		json += ",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += "]";
	return json;
}
template <typename T>
std::string dump_json_for_vector(const vector<T> &data)
{
	std::string json = "[";
	for (auto &&item : data)
	{
		json += item.dump_json();
		json += ",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += "]";
	return json;
}

} // namespace protolang