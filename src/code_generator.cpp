#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include "code_generator.h"
#include "log.h"

namespace protolang
{
void CodeGenerator::emit_object(const std::string &output_file)
{
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	auto target_triple = llvm::sys::getDefaultTargetTriple();
	this->module().setTargetTriple(
	    llvm::sys::getDefaultTargetTriple());
	std::string err;
	auto        target =
	    llvm::TargetRegistry::lookupTarget(target_triple, err);
	if (!target)
	{
		ErrorInternal e;
		e.message = err;
		throw std::move(e);
	}

	auto target_machine = target->createTargetMachine(
	    target_triple,
	    "generic",
	    "",
	    llvm::TargetOptions{},
	    std::optional<llvm::Reloc::Model>());

	this->module().setDataLayout(
	    target_machine->createDataLayout());

	std::error_code      ec;
	llvm::raw_fd_ostream dest(
	    output_file, ec, llvm::sys::fs::OF_None);

	if (ec)
	{
		ErrorWrite e;
		e.path = output_file;
		throw std::move(e);
	}

	llvm::legacy::PassManager pass;

	if (target_machine->addPassesToEmitFile(
	        pass, dest, nullptr, llvm::CGFT_ObjectFile))
	{
		ErrorInternal e;
		e.message = "Cannot emit file of this type.";
		throw std::move(e);
	}

	pass.run(this->module());
	dest.flush();
}
void CodeGenerator::gen(const std::string &output_file)
{
	try
	{
		this->emit_object(output_file);
	}
	catch (Error &e)
	{
		e.print(m_logger);
	}
}
} // namespace protolang