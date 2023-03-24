#include <concepts>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <llvm/IR/Type.h>
#include "builtin.h"
#include "code_generator.h"
#include "encoding.h"
#include "entity_system.h"
#include "env.h"
namespace protolang
{
static llvm::Value *scalar_cast(CodeGenerator &g,
                                llvm::Value   *input,
                                llvm::Type    *out_type);

llvm::Value *BuiltInVoidType::cast_inst_no_check(
    [[maybe_unused]] CodeGenerator &g,
    [[maybe_unused]] llvm::Value   *val,
    [[maybe_unused]] IType         *type)
{
	return nullptr;
}
StringU8 BuiltInVoidType::get_type_name()
{
	return u8"void";
}
StringU8 BuiltInVoidType::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"BuiltVoidType", "type":"{}"}})",
	    get_type_name());
}
llvm::Type *BuiltInVoidType::get_llvm_type(CodeGenerator &g)
{
	return llvm::Type::getVoidTy(g.context());
}
bool BuiltInVoidType::can_accept(IType *t)
{
	return t == static_cast<IType *>(this);
}
bool BuiltInVoidType::equal(IType *t)
{
	return t == static_cast<IType *>(this);
}

struct IScalarType : IType
{
	enum class ScalarKind
	{
		UInt,
		Int,
		Float,
		Double
	};

	virtual ScalarKind get_scalar_kind() const = 0;
	virtual unsigned   get_bits() const        = 0;

	bool can_accept(IType *other) override
	{
		if (auto scalar_type =
		        dynamic_cast<IScalarType *>(other))
			return this->get_scalar_kind() ==
			           scalar_type->get_scalar_kind() &&
			       this->get_bits() >= scalar_type->get_bits();
		else
			return false;
	}

	bool equal(IType *other) override
	{
		if (auto scalar_type =
		        dynamic_cast<IScalarType *>(other))
			return this->get_scalar_kind() ==
			           scalar_type->get_scalar_kind() &&
			       this->get_bits() == scalar_type->get_bits();
		else
			return false;
	}
};

template <unsigned bits, bool is_signed>
struct BuiltInIntType : IScalarType
{
	llvm::Value *cast_inst_no_check(
	    CodeGenerator          &g,
	    llvm::Value            *val,
	    [[maybe_unused]] IType *type) override
	{
		return scalar_cast(g, val, this->get_llvm_type(g));
	}

	StringU8 get_type_name() override
	{
		if constexpr (is_signed)
		{
			if constexpr (bits == 64)
				return "long";
			if constexpr (bits == 32)
				return "int";
			if constexpr (bits == 16)
				return "short";
			if constexpr (bits == 8)
				return "sbyte";
		}
		else
		{
			if constexpr (bits == 64)
				return "ulong";
			if constexpr (bits == 32)
				return "uint";
			if constexpr (bits == 16)
				return "ushort";
			if constexpr (bits == 8)
				return "byte";
		}
	}
	StringU8 dump_json() override
	{
		return fmt::format(
		    u8R"({{"obj":"BuiltInIntType", "type":"{}"}})",
		    get_type_name());
	}
	llvm::Type *get_llvm_type(CodeGenerator &g) override
	{
		return llvm::IntegerType::get(g.context(), bits);
	}
	ScalarKind get_scalar_kind() const override
	{
		return is_signed ? ScalarKind::Int : ScalarKind::UInt;
	}
	unsigned int get_bits() const override { return bits; }
};

struct BuiltInFloatType : IScalarType
{
	llvm::Value *cast_inst_no_check(
	    CodeGenerator          &g,
	    llvm::Value            *val,
	    [[maybe_unused]] IType *type) override
	{
		return scalar_cast(g, val, this->get_llvm_type(g));
	}

	StringU8 get_type_name() override
	{
		return StringU8("float");
	}
	StringU8 dump_json() override
	{
		return fmt::format(
		    u8R"({{"obj":"BuiltInFloatType", "type":"{}"}})",
		    get_type_name());
	}
	llvm::Type *get_llvm_type(CodeGenerator &g) override
	{
		return llvm::Type::getFloatTy(g.context());
	}
	ScalarKind get_scalar_kind() const override
	{
		return ScalarKind::Float;
	}
	unsigned int get_bits() const override { return 32; }
};

struct BuiltInDoubleType : IScalarType
{
	llvm::Value *cast_inst_no_check(
	    CodeGenerator          &g,
	    llvm::Value            *val,
	    [[maybe_unused]] IType *type) override
	{
		return scalar_cast(g, val, this->get_llvm_type(g));
	}

	StringU8 get_type_name() override
	{
		return StringU8("double");
	}
	StringU8 dump_json() override
	{
		return fmt::format(
		    u8R"({{"obj":"BuiltInDoubleType", "type":"{}"}})",
		    get_type_name());
	}
	llvm::Type *get_llvm_type(CodeGenerator &g) override
	{
		return llvm::Type::getDoubleTy(g.context());
	}
	ScalarKind get_scalar_kind() const override
	{
		return ScalarKind::Double;
	}
	unsigned int get_bits() const override { return 64; }
};

enum class ArithmaticType
{
	Add,
	Sub,
	Mul,
	Div
};

static const char *to_cstring(ArithmaticType t)
{
	switch (t)
	{
	case ArithmaticType::Add:
		return "add";
	case ArithmaticType::Sub:
		return "sub";
	case ArithmaticType::Mul:
		return "mul";
	case ArithmaticType::Div:
		return "div";
	}
	return nullptr;
}

template <ArithmaticType Ar>
struct BuiltInArithmetic : IOp
{

private:
	IScalarType *m_scalar_type;
	StringU8     m_mangled_name;

public:
	explicit BuiltInArithmetic(IScalarType *st)
	    : m_scalar_type(st)
	{}

	IType *get_return_type() override { return m_scalar_type; }
	size_t get_param_count() const override { return 2; }
	IType *get_param_type(size_t) override
	{
		return m_scalar_type;
	}
	StringU8 get_mangled_name() const override
	{
		return m_mangled_name;
	}
	void set_mangled_name(StringU8 name) override
	{
		m_mangled_name = std::move(name);
	}
	StringU8 dump_json() override
	{
		return fmt::format(u8"{}{}",
		                   to_cstring(Ar),
		                   m_scalar_type->get_type_name());
	}

	llvm::Value *gen_call(std::vector<llvm::Value *> args,
	                      CodeGenerator             &g) override
	{
		assert(args.size() == 2);
		switch (m_scalar_type->get_scalar_kind())
		{
		case IScalarType::ScalarKind::UInt:
			switch (Ar)
			{
			case ArithmaticType::Add:
				return g.builder().CreateNSWAdd(args[0],
				                                args[1]);
			case ArithmaticType::Sub:
				return g.builder().CreateNSWSub(args[0],
				                                args[1]);
			case ArithmaticType::Mul:
				return g.builder().CreateNSWMul(args[0],
				                                args[1]);
			case ArithmaticType::Div:
				return g.builder().CreateUDiv(args[0], args[1]);
			}
			break;
		case IScalarType::ScalarKind::Int:
			switch (Ar)
			{
			case ArithmaticType::Add:
				return g.builder().CreateNSWAdd(args[0],
				                                args[1]);
			case ArithmaticType::Sub:
				return g.builder().CreateNSWSub(args[0],
				                                args[1]);
			case ArithmaticType::Mul:
				return g.builder().CreateNSWMul(args[0],
				                                args[1]);
			case ArithmaticType::Div:
				return g.builder().CreateSDiv(args[0], args[1]);
			}
			break;
		case IScalarType::ScalarKind::Float:
		case IScalarType::ScalarKind::Double:
			switch (Ar)
			{
			case ArithmaticType::Add:
				return g.builder().CreateFAdd(args[0], args[1]);
			case ArithmaticType::Sub:
				return g.builder().CreateFSub(args[0], args[1]);
			case ArithmaticType::Mul:
				return g.builder().CreateFMul(args[0], args[1]);
			case ArithmaticType::Div:
				return g.builder().CreateFDiv(args[0], args[1]);
			}
			break;
		}
		return nullptr;
	}
};

template <std::derived_from<IType> Ty>
auto add_type(Env *env)
{
	auto     ptr     = make_uptr(new Ty{});
	auto     ptr_raw = ptr.get();
	StringU8 name    = ptr->get_type_name();
	env->add_keyword(name, std::move(ptr));
	return ptr_raw;
};

template <std::derived_from<IScalarType> ScTy>
void add_scalar_and_op(Env *env)
{
	auto ty_ptr = add_type<ScTy>(env);
	env->add(
	    Ident(u8"+", SrcRange()),
	    make_uptr(
	        new BuiltInArithmetic<ArithmaticType::Add>{ty_ptr}));
	env->add(
	    Ident(u8"-", SrcRange()),
	    make_uptr(
	        new BuiltInArithmetic<ArithmaticType::Sub>{ty_ptr}));
	env->add(
	    Ident(u8"*", SrcRange()),
	    make_uptr(
	        new BuiltInArithmetic<ArithmaticType::Mul>{ty_ptr}));
	env->add(
	    Ident(u8"/", SrcRange()),
	    make_uptr(
	        new BuiltInArithmetic<ArithmaticType::Div>{ty_ptr}));
}

void add_builtins(Env *env)
{
	add_type<BuiltInVoidType>(env);
	add_scalar_and_op<BuiltInIntType<32, true>>(env);
	add_scalar_and_op<BuiltInIntType<64, true>>(env);
	add_scalar_and_op<BuiltInIntType<32, false>>(env);
	add_scalar_and_op<BuiltInIntType<64, false>>(env);
	add_scalar_and_op<BuiltInFloatType>(env);
	add_scalar_and_op<BuiltInDoubleType>(env);
}

// 这个函数可以生成任何scalar之间的cast。
llvm::Value *scalar_cast(CodeGenerator &g,
                         llvm::Value   *input,
                         llvm::Type    *out_type)
{
	// double -> float
	if (input->getType()->isDoubleTy() && out_type->isFloatTy())
		return g.builder().CreateFPTrunc(input, out_type);

	// float -> double
	if (input->getType()->isFloatTy() && out_type->isDoubleTy())
		return g.builder().CreateFPExt(input, out_type);

	// int
	if (input->getType()->isIntegerTy() &&
	    out_type->isIntegerTy())
	{
		unsigned inputBitWidth =
		    input->getType()->getIntegerBitWidth();
		unsigned outputBitWidth = out_type->getIntegerBitWidth();
		if (inputBitWidth < outputBitWidth)
		{
			// 注意这里是哪种SExt还是ZExt
			return g.builder().CreateSExt(input, out_type);
		}
		else if (inputBitWidth > outputBitWidth)
		{
			return g.builder().CreateTrunc(input, out_type);
		}
		else
		{
			return g.builder().CreateBitCast(input, out_type);
		}
	}

	// pointer
	if (input->getType()->isPointerTy() &&
	    out_type->isPointerTy())
	{
		return g.builder().CreatePointerCast(input, out_type);
	}

	// 不支持
	return nullptr;
}

} // namespace protolang