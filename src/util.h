#pragma once
#include <string>
#include <vector>
template <typename T>
std::string dump_json_for_vector_of_ptr(const std::vector<T> &data)
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
std::string dump_json_for_vector(const std::vector<T> &data)
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
