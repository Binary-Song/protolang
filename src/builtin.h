#pragma once
#include <functional>
#include "entity_system.h"

namespace protolang
{
class Env;

struct BuiltInVoidType : IType
{
	llvm::Value *cast_inst_no_check(
	    CodeGenerator          &g,
	    llvm::Value            *val,
	    [[maybe_unused]] IType *type) override;
	StringU8 get_type_name() override;
	StringU8 dump_json() override;
	llvm::Type *get_llvm_type(CodeGenerator &g) override;
	bool        can_accept(IType *t) override;
	bool        equal(IType *t) override;
};

void add_builtins(Env *);
} // namespace protolang