#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "exceptions.h"
namespace protolang
{
/// 强制dyn_cast：
/// 失败会 throw 的 dynamic_cast
template <typename Derived, typename Base>
Derived dyn_cast_force(Base &&u)
{
	if (auto x = dynamic_cast<Derived>(u))
		return x;
	throw ExceptionCastError();
}

/// 失败会 throw 的 dynamic_cast
/// for unique_ptr
template <typename Derived, typename Base>
std::unique_ptr<Derived> dyn_cast_uptr_force(
    std::unique_ptr<Base> u)
{
	if (auto d = dynamic_cast<Derived *>(u.get()))
	{
		u.release();
		return std::unique_ptr<Derived>(d);
	}
	throw ExceptionCastError();
}

struct IJsonDumper
{
	virtual ~IJsonDumper()          = default;
	virtual StringU8 dump_json() = 0;
};

template <typename T>
StringU8 dump_json_for_vector_of_ptr(
    const std::vector<T> &data)
{
	StringU8 json = "[";
	for (auto &&item : data)
	{
		json += item->dump_json();
		json += u8",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += u8"]";
	return json;
}
template <typename T>
StringU8 dump_json_for_vector(const std::vector<T> &data)
{
	StringU8 json = "[";
	for (auto &&item : data)
	{
		json += item.dump_json();
		json += u8",";
	}
	if (json.ends_with(','))
	{
		json.pop_back();
	}
	json += u8"]";
	return json;
}

template <typename T>
bool all_equal(
    const std::vector<T> &v1,
    const std::vector<T> &v2,
    const std::function<bool(const T &e1, const T &e2)>
        &equal_func)
{
	if (v1.size() != v2.size())
		return false;
	for (size_t i = 0; i < v1.size(); i++)
	{
		if (equal_func(v1[i], v2[i]) == false)
		{
			return false;
		}
	}
	return true;
}

template <class T>
uptr<T> make_uptr(T *&&raw)
{
	return uptr<T>(raw);
}
} // namespace protolang