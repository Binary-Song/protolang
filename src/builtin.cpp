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

namespace builtin
{
struct IScalarType;
struct VoidType : IType
{
	llvm::Value *cast_inst_no_check(
	    CodeGenerator          &g,
	    llvm::Value            *val,
	    [[maybe_unused]] IType *type) override;
	StringU8    get_type_name() override;
	StringU8    dump_json() override;
	llvm::Type *get_llvm_type(CodeGenerator &g) override;
	bool        can_accept(IType *t) override;
	bool        equal(IType *t) override;
};

static llvm::Value *scalar_cast(CodeGenerator &g,
                                llvm::Value   *input,
                                IScalarType   *input_type,
                                IScalarType   *output_type);

llvm::Value *VoidType::cast_inst_no_check(
    [[maybe_unused]] CodeGenerator &g,
    [[maybe_unused]] llvm::Value   *val,
    [[maybe_unused]] IType         *type)
{
	return nullptr;
}
StringU8 VoidType::get_type_name()
{
	return u8"void";
}
StringU8 VoidType::dump_json()
{
	return fmt::format(
	    u8R"({{"obj":"BuiltVoidType", "type":"{}"}})",
	    get_type_name());
}
llvm::Type *VoidType::get_llvm_type(CodeGenerator &g)
{
	return llvm::Type::getVoidTy(g.context());
}
bool VoidType::can_accept(IType *t)
{
	return t == static_cast<IType *>(this);
}
bool VoidType::equal(IType *t)
{
	return t == static_cast<IType *>(this);
}

struct IScalarType : IType
{
	enum class ScalarKind
	{
		Bool,
		UInt,
		Int,
		Fp,
	};

	virtual ScalarKind get_scalar_kind() const = 0;
	virtual unsigned   get_bits() const        = 0;

	bool can_accept(IType *other) override
	{
		// 能隐式cast的只有同类型、放大位数
		if (auto scalar_type =
		        dynamic_cast<IScalarType *>(other))
			return this->get_scalar_kind() ==
			           scalar_type->get_scalar_kind() &&
			       this->get_bits() >= scalar_type->get_bits();
		else
			return false;
	}

	bool can_accept_explicit_cast_no_check(IType *t) override
	{
		// 标量类型之间都可以互相强转
		return dynamic_cast<IScalarType *>(t);
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

	llvm::Value *cast_inst_no_check(CodeGenerator &g,
	                                llvm::Value   *val,
	                                IType         *type) override
	{
		return scalar_cast(g,
		                   val,
		                   dyn_cast_force<IScalarType *>(type),
		                   dyn_cast_force<IScalarType *>(this));
	}
};

struct BoolType : IScalarType
{
	bool        can_accept(IType *iType) override;
	bool        equal(IType *iType) override;
	StringU8    get_type_name() override;
	llvm::Type *get_llvm_type(CodeGenerator &g) override;
	StringU8    dump_json() override;

public:
	ScalarKind   get_scalar_kind() const override;
	unsigned int get_bits() const override;
};
template <unsigned bits, bool is_signed>
struct IntType : IScalarType
{

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

struct FloatType : IScalarType
{

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
		return ScalarKind::Fp;
	}
	unsigned int get_bits() const override { return 32; }
};

struct DoubleType : IScalarType
{

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
		return ScalarKind::Fp;
	}
	unsigned int get_bits() const override { return 64; }
};
enum class OperationType
{
	Add = 1,
	Sub,
	Mul,
	Div,

	Eq = 100,
	Ne,
	Gt,
	Lt,
	Ge,
	Le
};

constexpr bool is_arith(OperationType t)
{
	return int(t) >= 1 && int(t) < 100;
}

constexpr bool is_compare(OperationType t)
{
	return int(t) >= 100 && int(t) < 200;
}

static const char *to_cstring(OperationType t)
{
	switch (t)
	{
	case OperationType::Add:
		return "add";
	case OperationType::Sub:
		return "sub";
	case OperationType::Mul:
		return "mul";
	case OperationType::Div:
		return "div";
	case OperationType::Eq:
		return "eq";
	case OperationType::Ne:
		return "ne";
	case OperationType::Gt:
		return "gt";
	case OperationType::Lt:
		return "lt";
	case OperationType::Ge:
		return "ge";
	case OperationType::Le:
		return "le";
	}
	return nullptr;
}

template <OperationType Ar>
struct Operator : IOp
{

private:
	IScalarType *m_scalar_type;
	IType       *m_bool_type;
	StringU8     m_mangled_name;

public:
	explicit Operator(IScalarType *scalar_type, IType *bool_type)
	    : m_scalar_type(scalar_type)
	    , m_bool_type(bool_type)
	{}

	IType *get_return_type() override
	{
		if constexpr (is_arith(Ar))
			return m_scalar_type;
		else if (is_compare(Ar))
			return m_bool_type;
		throw ExceptionNotImplemented{};
	}
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
		                   as_u8(to_cstring(Ar)),
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
			case OperationType::Add:
				return g.builder().CreateNSWAdd(args[0],
				                                args[1]);
			case OperationType::Sub:
				return g.builder().CreateNSWSub(args[0],
				                                args[1]);
			case OperationType::Mul:
				return g.builder().CreateNSWMul(args[0],
				                                args[1]);
			case OperationType::Div:
				return g.builder().CreateUDiv(args[0], args[1]);

			case OperationType::Eq:
				return g.builder().CreateICmpEQ(args[0],
				                                args[1]);

			case OperationType::Ne:
				return g.builder().CreateICmpNE(args[0],
				                                args[1]);

			case OperationType::Lt:
				return g.builder().CreateICmpULT(args[0],
				                                 args[1]);

			case OperationType::Gt:
				return g.builder().CreateICmpUGT(args[0],
				                                 args[1]);

			case OperationType::Le:
				return g.builder().CreateICmpULE(args[0],
				                                 args[1]);

			case OperationType::Ge:
				return g.builder().CreateICmpUGE(args[0],
				                                 args[1]);
			}
			break;
		case IScalarType::ScalarKind::Int:
			switch (Ar)
			{
			case OperationType::Add:
				return g.builder().CreateNSWAdd(args[0],
				                                args[1]);
			case OperationType::Sub:
				return g.builder().CreateNSWSub(args[0],
				                                args[1]);
			case OperationType::Mul:
				return g.builder().CreateNSWMul(args[0],
				                                args[1]);
			case OperationType::Div:
				return g.builder().CreateSDiv(args[0], args[1]);

			case OperationType::Eq:
				return g.builder().CreateICmpEQ(args[0],
				                                args[1]);

			case OperationType::Ne:
				return g.builder().CreateICmpNE(args[0],
				                                args[1]);

			case OperationType::Lt:
				return g.builder().CreateICmpSLT(args[0],
				                                 args[1]);

			case OperationType::Gt:
				return g.builder().CreateICmpSGT(args[0],
				                                 args[1]);

			case OperationType::Le:
				return g.builder().CreateICmpSLE(args[0],
				                                 args[1]);

			case OperationType::Ge:
				return g.builder().CreateICmpSGE(args[0],
				                                 args[1]);
			}
			break;
		case IScalarType::ScalarKind::Fp:
			switch (Ar)
			{
			case OperationType::Add:
				return g.builder().CreateFAdd(args[0], args[1]);
			case OperationType::Sub:
				return g.builder().CreateFSub(args[0], args[1]);
			case OperationType::Mul:
				return g.builder().CreateFMul(args[0], args[1]);
			case OperationType::Div:
				return g.builder().CreateFDiv(args[0], args[1]);

			case OperationType::Eq:
				return g.builder().CreateFCmpOEQ(args[0],
				                                 args[1]);

			case OperationType::Ne:
				return g.builder().CreateFCmpONE(args[0],
				                                 args[1]);

			case OperationType::Lt:
				return g.builder().CreateFCmpOLT(args[0],
				                                 args[1]);

			case OperationType::Gt:
				return g.builder().CreateFCmpOGT(args[0],
				                                 args[1]);

			case OperationType::Le:
				return g.builder().CreateFCmpOLE(args[0],
				                                 args[1]);

			case OperationType::Ge:
				return g.builder().CreateFCmpOGE(args[0],
				                                 args[1]);
			}
			break;
		case IScalarType::ScalarKind::Bool:
			switch (Ar)
			{
			case OperationType::Eq:
				return g.builder().CreateICmpEQ(args[0],
				                                args[1]);
			case OperationType::Ne:
				return g.builder().CreateICmpNE(args[0],
				                                args[1]);
			default:
				return nullptr;
			}
		}
		return nullptr;
	}
};

// 这个函数可以生成任何scalar之间的cast。
llvm::Value *scalar_cast(CodeGenerator &g,
                         llvm::Value   *input,
                         IScalarType   *input_type,
                         IScalarType   *output_type)
{ 
	assert(input->getType() == input_type->get_llvm_type(g));

	// auto input_llvm_type  = input_type->get_llvm_type(g);
	auto output_llvm_type = output_type->get_llvm_type(g);

	auto input_kind  = input_type->get_scalar_kind();
	auto output_kind = output_type->get_scalar_kind();

	// float <-> double
	if (input_kind == IScalarType::ScalarKind::Fp &&
	    output_kind == IScalarType::ScalarKind::Fp)
	{
		if (input_type->get_bits() < output_type->get_bits())
			return g.builder().CreateFPExt(input,
			                               output_llvm_type);
		else
			return g.builder().CreateFPTrunc(input,
			                                 output_llvm_type);
	}

	// int <-> long
	if (input_kind == IScalarType::ScalarKind::Int &&
	    output_kind == IScalarType::ScalarKind::Int)
	{
		if (input_type->get_bits() < output_type->get_bits())
		{
			return g.builder().CreateSExt(input,
			                              output_llvm_type);
		}
		else
		{
			return g.builder().CreateTrunc(input,
			                               output_llvm_type);
		}
	}

	// uint <-> ulong
	if (input_kind == IScalarType::ScalarKind::UInt &&
	    output_kind == IScalarType::ScalarKind::UInt)
	{
		if (input_type->get_bits() < output_type->get_bits())
		{
			return g.builder().CreateZExt(input,
			                              output_llvm_type);
		}
		else
		{
			return g.builder().CreateTrunc(input,
			                               output_llvm_type);
		}
	}

	// fp -> int
	if (input_kind == IScalarType::ScalarKind::Fp &&
	    output_kind == IScalarType::ScalarKind::Int)
	{
		return g.builder().CreateFPToSI(input, output_llvm_type);
	}

	// fp -> uint
	if (input_kind == IScalarType::ScalarKind::Fp &&
	    output_kind == IScalarType::ScalarKind::UInt)
	{
		return g.builder().CreateFPToUI(input, output_llvm_type);
	}

	// int -> fp
	if (input_kind == IScalarType::ScalarKind::Int &&
	    output_kind == IScalarType::ScalarKind::Fp)
	{
		return g.builder().CreateSIToFP(input, output_llvm_type);
	}

	// uint -> fp
	if (input_kind == IScalarType::ScalarKind::UInt &&
	    output_kind == IScalarType::ScalarKind::Fp)
	{
		return g.builder().CreateUIToFP(input, output_llvm_type);
	}

	// 不支持
	return nullptr;
}

bool BoolType::can_accept(IType *iType)
{
	return equal(iType);
}
bool BoolType::equal(IType *iType)
{
	return this == dynamic_cast<BoolType *>(iType);
}
StringU8 BoolType::get_type_name()
{
	return "bool";
}
llvm::Type *BoolType::get_llvm_type(CodeGenerator &g)
{
	return llvm::Type::getInt1Ty(g.context());
}
StringU8 BoolType::dump_json()
{
	return "BoolType";
}

IScalarType::ScalarKind BoolType::get_scalar_kind() const
{
	return ScalarKind::Bool;
}
unsigned int BoolType::get_bits() const
{
	return 1;
}

} // namespace builtin
using namespace builtin;

template <std::derived_from<IType> Ty>
auto add_type(Env *env)
{
	auto     ptr     = make_uptr(new Ty{});
	auto     ptr_raw = ptr.get();
	StringU8 name    = ptr->get_type_name();
	env->add_keyword(name, std::move(ptr));
	return ptr_raw;
}

template <std::derived_from<IScalarType> ScTy>
void add_scalar_and_op(Env *env, IType *bool_type)
{
	auto ty_ptr = add_type<ScTy>(env);
	env->add(Ident(u8"+", SrcRange()),
	         make_uptr(new Operator<OperationType::Add>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"-", SrcRange()),
	         make_uptr(new Operator<OperationType::Sub>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"*", SrcRange()),
	         make_uptr(new Operator<OperationType::Mul>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"/", SrcRange()),
	         make_uptr(new Operator<OperationType::Div>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"==", SrcRange()),
	         make_uptr(new Operator<OperationType::Eq>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"!=", SrcRange()),
	         make_uptr(new Operator<OperationType::Ne>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8">", SrcRange()),
	         make_uptr(new Operator<OperationType::Gt>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"<", SrcRange()),
	         make_uptr(new Operator<OperationType::Lt>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8">=", SrcRange()),
	         make_uptr(new Operator<OperationType::Ge>{
	             ty_ptr, bool_type}));
	env->add(Ident(u8"<=", SrcRange()),
	         make_uptr(new Operator<OperationType::Le>{
	             ty_ptr, bool_type}));
}

void add_builtins(Env *env)
{
	// void
	add_type<VoidType>(env);
	// bool
	auto bool_type = add_type<BoolType>(env);
	//   bool ==
	env->add(Ident(u8"==", SrcRange()),
	         make_uptr(new Operator<OperationType::Eq>{
	             bool_type, bool_type}));
	//   bool !=
	env->add(Ident(u8"!=", SrcRange()),
	         make_uptr(new Operator<OperationType::Ne>{
	             bool_type, bool_type}));
	// scalars
	add_scalar_and_op<IntType<32, true>>(env, bool_type);
	add_scalar_and_op<IntType<64, true>>(env, bool_type);
	add_scalar_and_op<IntType<32, false>>(env, bool_type);
	add_scalar_and_op<IntType<64, false>>(env, bool_type);
	add_scalar_and_op<FloatType>(env, bool_type);
	add_scalar_and_op<DoubleType>(env, bool_type);
}

} // namespace protolang