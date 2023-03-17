//
// Created by yeh18 on 2023/3/12.
//

#ifndef PROTOLANG_BUILTIN_H
#define PROTOLANG_BUILTIN_H
#include "entity_system.h"
namespace protolang
{

struct BuiltInInt : IType
{
	static uptr<BuiltInInt> s_instance;

	static BuiltInInt *get_instance()
	{
		return s_instance.get();
	}

	bool can_accept(IType *other) override
	{
		return equal(other);
	}
	bool equal(IType *other) override
	{
		return dynamic_cast<const BuiltInInt *>(other);
	}
	std::string get_type_name() override { return "int"; }
	std::string dump_json() override
	{
		return R"({"obj":"BuiltInInt"})";
	}
};

struct BuiltInFloat : IType
{
	static uptr<BuiltInFloat> s_instance;
	static BuiltInFloat      *get_instance()
	{
		return s_instance.get();
	}

	bool can_accept(IType *other) override
	{
		return equal(other);
	}
	bool equal(IType *other) override
	{
		return dynamic_cast<const BuiltInFloat *>(other);
	}
	std::string get_type_name() override { return "float"; }
	std::string dump_json() override
	{
		return R"({"obj":"BuiltInFloat"})";
	}
};
struct BuiltInDouble : IType
{
	static uptr<BuiltInDouble> s_instance;
	static BuiltInDouble      *get_instance()
	{
		return s_instance.get();
	}

	bool can_accept(IType *other) override
	{
		return equal(other);
	}
	bool equal(IType *other) override
	{
		return dynamic_cast<const BuiltInDouble *>(other);
	}
	std::string get_type_name() override { return "double"; }
	std::string dump_json() override
	{
		return R"({"obj":"BuiltInDouble"})";
	}
};
struct BuiltInFuncBody : IFuncBody
{};

template <typename T>
struct BuiltInAdd : IFunc
{
private:
	BuiltInFuncBody body;

public:
	IType *get_return_type() override
	{
		return T::get_instance();
	}
	size_t get_param_count() const override { return 2; }
	IType *get_param_type(size_t) override
	{
		return T::get_instance();
	}
	IFuncBody  *get_body() override { return &body; }
	std::string dump_json() override
	{
		return R"({"obj":"BuiltInAdd"})";
	}
};

} // namespace protolang

#endif // PROTOLANG_BUILTIN_H
