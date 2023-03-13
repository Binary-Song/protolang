#pragma once
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <memory>
namespace protolang
{

struct CodeGenerator
{
private:
	std::unique_ptr<llvm::LLVMContext>   m_context;
	std::unique_ptr<llvm::IRBuilder<>>   m_builder;
	std::unique_ptr<llvm::Module>        m_module;
	std::map<std::string, llvm::Value *> m_named_values;

public:
	explicit CodeGenerator(const std::string &module_name)
	    : m_context(std::make_unique<llvm::LLVMContext>())
	    , m_builder(
	          std::make_unique<llvm::IRBuilder<>>(*m_context))
	    , m_module(std::make_unique<llvm::Module>(module_name,
	                                              *m_context))
	{}

	[[nodiscard]] llvm::LLVMContext &context()
	{
		return *m_context.get();
	}
	[[nodiscard]] llvm::IRBuilder<> &builder()
	{
		return *m_builder.get();
	}
	[[nodiscard]] llvm::Module &module()
	{
		return *m_module.get();
	}
	[[nodiscard]] llvm::Value *get_named_value(
	    const std::string &key) const
	{
		return m_named_values.at(key);
	}
};

} // namespace protolang
