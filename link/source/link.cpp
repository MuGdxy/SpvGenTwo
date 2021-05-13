#include "spvgentwo/Module.h"
#include "spvgentwo/Grammar.h"
#include "spvgentwo/Templates.h"

#include "common/BinaryFileReader.h"
#include "common/BinaryFileWriter.h"
#include "common/ConsoleLogger.h"
#include "common/HeapList.h"
#include "common/HeapString.h"
#include "common/ModulePrinter.h"
#include "common/LinkerHelper.h"

#include <cstring> // strcmp
#include <cstdio> // printf, note windows console, and others too, don't print SPIR-V's UTF-8 strings properly
#include <cstdlib> // strtoul
#include <cctype> // to lower

using namespace spvgentwo;
using namespace ModulePrinter;

auto g_printer = ModuleSimpleFuncPrinter([](const char* _pStr) { printf("%s", _pStr); });
ConsoleLogger g_logger;
HeapAllocator g_alloc;
const Grammar g_gram(&g_alloc);

// import/export target
struct Target
{
	Instruction* instr = nullptr;
	const char* name = nullptr;
	spv::LinkageType type = spv::LinkageType::Max;
	bool exportAllReferencedVars = false; // for instr == OpFunction export
};

int patch(Module& _module, const HeapList<Target>& _targets, const char* _out)
{
	_module.addCapability(spv::Capability::Linkage);

	for (auto& t : _targets)
	{
		if (*t.instr == spv::Op::OpFunction && t.instr->getFunction()->isEntryPoint() == false)
		{
			Function* func = t.instr->getFunction();

			if (t.type == spv::LinkageType::Import)
			{
				if (func->empty() == false)
				{
					g_logger.logInfo("Functions marked for Import must not contain any basic blocks, removing %u blocks", func->size());
					LinkerHelper::removeFunctionBody(*func);
				}
			}
			else if (t.type == spv::LinkageType::Export && t.exportAllReferencedVars)
			{
				if (LinkerHelper::addLinkageDecorateForUsedGlobalVariables(*func, t.type, &g_alloc) == false)
				{
					return -1;
				}
			}
		}

		if (*t.instr != spv::Op::OpFunction && *t.instr != spv::Op::OpVariable)
		{
			g_logger.logError("Only OpFunctions which are not EntryPoints and global OpVariables can be imported/exported [%s]", g_gram.getInfo(static_cast<unsigned int>(t.instr->getOperation())));
			return -1;
		}

		Instruction* instr = _module.addDecorationInstr();
		instr->opDecorate(t.instr, spv::Decoration::LinkageAttributes, t.name, t.type);
		g_printer << "Added "; printInstruction(*instr, g_gram, g_printer); g_printer << "\n";
	}

	BinaryFileWriter writer(_out);
	if (writer.isOpen() == false)
	{
		g_logger.logError("Failed to open %s", _out);
		return -1;
	}

	_module.finalizeAndWrite(writer);
	return 0;
}

int link(HeapList<Module>& _libs, Module& _target, const LinkerHelper::LinkerOptions& _options, const char* _out)
{
	for (const Module& lib : _libs)
	{
		if (LinkerHelper::import(lib, _target, _options) == false)
		{
			return -1;
		}
	}

	BinaryFileWriter writer(_out);
	if (writer.isOpen() == false)
	{
		g_logger.logError("Failed to open %s", _out);
		return -1;
	}

	_target.finalizeEntryPoints();

	// don't need to call assignIDs, we are using AssignResultIDs
	if (_target.write(writer) == false) 
	{
		g_logger.logError("Failed to write %s", _out);
		return -1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	using namespace LinkerHelper;

	struct CMDOption
	{
		using Func = void(LinkerOptions& _options);
		const char* str;
		const char* alt;
		Func* func = nullptr;
	};

#define FLAGOPTION(x, acronym) {"--" #x, "--"#acronym, [](LinkerOptions& _options){_options.flags |= LinkerOptionBits::x;}}

	const CMDOption CmdOptions[] =
	{
		FLAGOPTION(ImportMissingTypes, types),
		FLAGOPTION(ImportMissingConstants, constants),
		FLAGOPTION(ImportReferencedDecorations, refdecos),
		FLAGOPTION(ImportReferencedNames, refnames),
		FLAGOPTION(ImportReferencedFunctions, reffuncs),
		FLAGOPTION(ImportReferencedVariables, refvars),
		//FLAGOPTION(AssignResultIDs, asid), // already on by default
		FLAGOPTION(RemoveLinkageCapability, rmcap),
		FLAGOPTION(AutoAddRequiredCapabilitiesAndExtensions, addcaps),
		FLAGOPTION(UpdateEntryPointGlobalVarInterface, variface),
		FLAGOPTION(CopyOpSourceStringInstructions, srcstrings),
		FLAGOPTION(CopyOpLineInstructions, lines),
		FLAGOPTION(CopyOpModuleProcessedInstructions, processed),
		{
			"--PrintInstructions",
			"--verbose",
			[](LinkerOptions& _options){
			_options.grammar = &g_gram;
			_options.printer = &g_printer;
			}
		}
	};

	const char* out = nullptr;
	const char* patchspv = nullptr;
	const char* targetspv = nullptr;
	
	HeapList<Target> targets;
	HeapList<Module> libs;
	LinkerOptions options{};

	options.flags = LinkerOptionBits::AssignResultIDs;
	// we dont want to set LinkerOptionBits::UpdateEntryPointGlobalVarInterface, we call finalizeEntryPoints() manually 

	auto addOption = [&options, &CmdOptions](const char* arg) -> bool
	{
		auto casecmp = [](const char* l, const char* r)
		{
			while(*l != 0 && *r != 0)
			{
				if (tolower(*l) != tolower(*r))
					return false;
				++l; ++r;
			}
			return true;
		};
		for(const CMDOption& o : CmdOptions)
		{
			if(casecmp(arg, o.alt) || casecmp(arg, o.str))
			{
				g_logger.logInfo("Option: %s", arg);
				o.func(options);
				return true;
			}
		}
		return false;
	};

	Module patchModule(&g_alloc, &g_logger);
	Module targetModule(&g_alloc, &g_logger);

	int i = 1u;
	auto addTarget = [&](spv::LinkageType type)
	{
		Instruction* instr = nullptr;
		const char* target = argv[++i];
		if (auto id = strtoul(target, nullptr, 10); id != 0u && id != sgt_uint32_max)
		{
			instr = patchModule.getInstructionById(spv::Id{ id });
		}
		else
		{
			instr = patchModule.getInstructionByName(target);
		}

		if (instr == nullptr)
		{
			g_logger.logError("Could not find %s in %s", target, patchspv);
			return;
		}

		if (const char* name = argv[++i]; name != nullptr)
		{
			bool exportVars = false;
			if (type == spv::LinkageType::Export && i + 1 < argc && (strcmp(argv[i + 1], "--vars") == 0))
			{
				++i;
				exportVars = true;
			}

			targets.emplace_back(instr, name, type, exportVars);
			g_logger.logInfo("Adding %s target instruction \'%s\' with name \'%s\'", type == spv::LinkageType::Import ? "import" : "export", target, name);
		}
	};

	for (; i < argc; ++i)
	{
		const char* arg = argv[i];
		if (i + 1 < argc && (strcmp(arg, "--p") == 0 || strcmp(arg, "--patchspv") == 0))
		{
			patchspv = argv[++i];
			BinaryFileReader reader(g_alloc, patchspv);
			if (reader == false)
			{
				g_logger.logError("Failed to open %s", patchspv);
				return -1;
			}
			else if (patchModule.readAndInit(reader, g_gram) == false)
			{
				return -1;
			}
		}
		else if (i + 1 < argc && (strcmp(arg, "--l") == 0 || strcmp(arg, "--lib") == 0))
		{
			const char* file = argv[++i];
			BinaryFileReader reader(g_alloc, file);
			if (reader == false)
			{
				g_logger.logError("Failed to open %s", file);
				return -1;
			}
			else if (libs.emplace_back(&g_alloc, &g_logger).readAndInit(reader, g_gram) == false)
			{
				return -1;
			}
		}
		else if (i + 1 < argc && (strcmp(arg, "--t") == 0 || strcmp(arg, "--target") == 0))
		{
			targetspv = argv[++i];
			BinaryFileReader reader(g_alloc, targetspv);
			if (reader == false)
			{
				g_logger.logError("Failed to open %s", targetspv);
				return -1;
			}
			else if (targetModule.readAndInit(reader, g_gram) == false)
			{
				return -1;
			}
		}
		else if (i + 1 < argc && (strcmp(arg, "--o") == 0 || strcmp(arg, "--out") == 0))
		{
			out = argv[++i];
			g_logger.logInfo("Output file \'%s\'", out);
		}
		else if (i + 1 < argc && (strcmp(arg, "--e") == 0 || strcmp(arg, "--export") == 0))
		{
			addTarget(spv::LinkageType::Export);
		}
		else if (i + 1 < argc && (strcmp(arg, "--i") == 0 || strcmp(arg, "--import") == 0))
		{
			addTarget(spv::LinkageType::Import);
		}
		else
		{
			if (addOption(arg) == false)
			{
				g_logger.logWarning("Unkown parameter %s", arg);
			}
		}
	}

	if (targets.empty() == false)
	{
		return patch(patchModule, targets, out != nullptr ? out : patchspv);
	}

	if (libs.empty() == false)
	{
		return link(libs, targetModule, options, out != nullptr ? out : targetspv);
	}

	return 0;
}