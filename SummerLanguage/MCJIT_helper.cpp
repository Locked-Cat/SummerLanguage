#include "MCJIT_helper.h"

namespace summer_lang
{
	MCJIT_helper::~MCJIT_helper()
	{
		if (open_module_)
			delete open_module_;

		for (auto i = engines_.begin(); i != engines_.end(); ++i)
			delete *i;
	}

	llvm::Function * MCJIT_helper::get_function(const std::string & name)
	{
		for (auto i = modules_.begin(); i != modules_.end(); ++i)
		{
			auto function = (*i)->getFunction(name);
			if (function)
			{
				if (*i == open_module_)
					return function;

				auto new_function = open_module_->getFunction(name);
				if (new_function && !new_function->empty())
					throw std::exception("Redefinition of function across modules");

				if (!new_function)
					new_function = llvm::Function::Create(function->getFunctionType(), llvm::Function::ExternalLinkage, name, open_module_);
			}
		}
		return nullptr;
	}

	llvm::Module * MCJIT_helper::get_module_for_new_function()
	{
		if (!open_module_)
		{
			open_module_ = new llvm::Module("mcjit_module", context_);
			open_module_->setTargetTriple("i686-pc-windows-msvc-elf");
			modules_.push_back(open_module_);
		}
		return open_module_;
	}

	void * MCJIT_helper::get_pointer_to_function(llvm::Function * function)
	{
		for (auto i = engines_.begin(); i != engines_.end(); ++i)
		{
			if (auto addr = (*i)->getPointerToFunction(function))
				return addr;
		}

		if (open_module_)
		{
			std::string error_str;
			auto new_engine = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(open_module_))
				.setErrorStr(&error_str)
				.setMCJITMemoryManager(std::make_unique<HelpingMemoryManager>(this))
				.create();
			if (!new_engine)
			{
				throw std::exception(("Can't create Execution Engine: " + error_str).c_str());
				exit(1);
			}

			auto fpm = new llvm::legacy::FunctionPassManager(open_module_);
			open_module_->setDataLayout(*new_engine->getDataLayout());
			fpm->add(llvm::createBasicAliasAnalysisPass());
			fpm->doInitialization();

			for (auto i = open_module_->begin(); i != open_module_->end(); ++i)
				fpm->run(*i);

			delete fpm;
			open_module_ = nullptr;
			engines_.push_back(new_engine);
			new_engine->finalizeObject();
			return new_engine->getPointerToFunction(function);
		}

		return nullptr;
	}

	void * MCJIT_helper::get_symbol_address(const std::string & name)
	{
		for (auto i = engines_.begin(); i != engines_.end(); ++i)
		{
			if (auto addr = (*i)->getFunctionAddress(name))
				return (void *)addr;
		}
		return nullptr;
	}

	std::string MCJIT_helper::generate_function_name(const std::string & name)
	{
		if (!name.length())
			return "anno_func";
		return name;
	}

	uint64_t HelpingMemoryManager::getSymbolAddress(const std::string & name)
	{
		auto p_func = llvm::SectionMemoryManager::getSymbolAddress(name);
		if (p_func)
			return p_func;

		p_func = (uint64_t)helper_->get_symbol_address(name);
		if (!p_func)
			throw std::exception(("Program used extern function '" + name + "' which could not be resolved!").c_str());

		return p_func;
	}
}
