#pragma once

#include "Function.h"

namespace spvgentwo
{
	enum class GlobalInterfaceVersion
	{
		SpirV1_3, // 1.0 - 1.3 Input and Output StorageClass
		SpirV14_x // 1.4 - 1.x any StorageClass != Function
	};

	class EntryPoint : public Function
	{
		friend class Module;
	public:
		EntryPoint() = default;

		// empty function, call setReturnType() first, then use addParameters() and then finalize() to create the function
		EntryPoint(Module* _pModule);

		// creates the whole function signature, finalize() does NOT need to be called
		template <class ... TypeInstr>
		EntryPoint(Module* _pModule, const spv::ExecutionModel _model, const char* _pEntryPointName, const Flag<spv::FunctionControlMask> _control, Instruction* _pReturnType, TypeInstr* ... _paramTypeInstructions);
		
		~EntryPoint() override;

		// TODO: move constructor & asignment

		// get all the global OpVariables with StorageClass != Function used in this function
		void getGlobalVariableInterface(List<Operand>& _outVarinstr, const GlobalInterfaceVersion _version) const;

		spv::ExecutionModel getExecutionModel() const { return m_ExecutionModel; }
		void setExecutionModel(const spv::ExecutionModel _model) { m_ExecutionModel = _model; }

		// OpEntryPoint
		Instruction* getEntryPoint() { return &m_EntryPoint; }
		const Instruction* getEntryPoint() const { return &m_EntryPoint; }

		template <class ... Args>
		Instruction* addExecutionMode(const spv::ExecutionMode _mode, Args ... _args);

		// overrides Functions finalize (used internally), _pEntryPointName is mandatory parameter, returns opFunction
		Instruction* finalize(const spv::ExecutionModel _model, const Flag<spv::FunctionControlMask> _control, const char* _pEntryPointName);

	private:
		// only to be called by the Module before serialization
		void finalizeGlobalInterface(const GlobalInterfaceVersion _version);

	private:
		Instruction m_EntryPoint; // OpEntryPoint
		spv::ExecutionModel m_ExecutionModel = spv::ExecutionModel::Max;
		bool m_finalized = false;
	};

	template<class ...TypeInstr>
	inline EntryPoint::EntryPoint(Module* _pModule, const spv::ExecutionModel _model, const char* _pEntryPointName, const Flag<spv::FunctionControlMask> _control, Instruction* _pReturnType, TypeInstr* ..._paramTypeInstructions) :
		Function(_pModule, _pEntryPointName, _control, _pReturnType, _paramTypeInstructions...),
		m_EntryPoint(this),
		m_ExecutionModel(_model)
	{
		m_EntryPoint.opEntryPoint(_model, &m_Function, _pEntryPointName);
	}

	template<class ...Args>
	inline Instruction* EntryPoint::addExecutionMode(const spv::ExecutionMode _mode, Args ..._args)
	{
		Instruction* pInstr = getModule()->addExtensionModeInstr();

		if (getExecutionModeOp(_mode) == spv::Op::OpExecutionModeId)
		{
			if constexpr (sizeof...(_args) > 0 && stdrep::conjunction_v<stdrep::is_same<Instruction, Args>...>)
			{
				pInstr->opExecutionModeId(&m_Function, _mode, _args...);			
			}
			else if (sizeof...(_args) == 0)
			{
				pInstr->opExecutionModeId(&m_Function, _mode);
			}
		}
		else
		{
			if constexpr (sizeof...(_args) > 0 && false == stdrep::conjunction_v<stdrep::is_same<Instruction, Args>...>)
			{
				pInstr->opExecutionMode(&m_Function, _mode, _args...);
			}
			else if (sizeof...(_args) == 0)
			{
				pInstr->opExecutionMode(&m_Function, _mode);
			}
		}

		return pInstr;
	}
} // !spvgentwo