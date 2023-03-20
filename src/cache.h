#pragma once
#include <functional>
namespace protolang
{
template <typename Data, bool allow_empty = false>
class Cache
{
private:
	Data                   *data_ptr = nullptr;
	std::function<Data *()> update_func;

public:
	explicit Cache(std::function<Data *()> update_func)
	    : update_func(std::move(update_func))
	{}

	Data *get()
	{
		if (data_ptr == nullptr)
		{
			data_ptr = update_func();
			if (!allow_empty)
			{
				assert(data_ptr);
			}
		}
		return data_ptr;
	}

	void set(Data *new_val) { data_ptr = new_val; }
};

} // namespace protolang