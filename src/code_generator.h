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
};

} // namespace protolang
