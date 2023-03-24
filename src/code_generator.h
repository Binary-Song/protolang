#pragma once
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <memory>
#include "encoding.h"

namespace protolang
{
class Logger;
struct CodeGenerator
{
private:
	std::unique_ptr<llvm::LLVMContext> m_context;
	std::unique_ptr<llvm::IRBuilder<>> m_builder;
	std::unique_ptr<llvm::Module>      m_module;
	std::map<StringU8, llvm::Value *>  m_named_values;
	Logger                            &m_logger;

public:
	explicit CodeGenerator(Logger         &logger,
	                       const StringU8 &module_name)
	    : m_logger(logger)
	    , m_context(std::make_unique<llvm::LLVMContext>())
	    , m_builder(
	          std::make_unique<llvm::IRBuilder<>>(*m_context))
	    , m_module(std::make_unique<llvm::Module>(
	          as_str(module_name), *m_context))
	{}

	void gen(const StringU8 &output_file);

	llvm::LLVMContext &context() { return *m_context; }
	llvm::IRBuilder<> &builder() { return *m_builder; }
	llvm::Module      &module() { return *m_module; }
	llvm::Value       *get_named_value(const StringU8 &key) const
	{
		return m_named_values.at(key);
	}

private:
	void emit_object(const StringU8 &output_file);
};

} // namespace protolang
