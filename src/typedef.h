#pragma once
#include <cctype>
#include <memory>
namespace protolang
{

using i32 = std::int32_t;
using u32 = std::uint32_t;

using i64 = std::int64_t;
using u64 = std::uint64_t;

template<typename T>
using uptr = std::unique_ptr<T>;

} // namespace protolang