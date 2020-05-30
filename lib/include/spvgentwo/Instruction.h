#pragma once

#include "List.h"
#include "Operand.h"
#include "Flag.h"

namespace spvgentwo
{
	// forward delcs
	class Type;
	class Function;
	class Module;

	class Instruction : public List<Operand>
	{
		friend class Module;
		friend class BasicBlock;

	public:
		enum class ParentType
		{
			BasicBlock,
			Function,
			Module
		};

	private:
		spv::Op m_Operation = spv::Op::OpNop;

		ParentType m_parentType = ParentType::BasicBlock;

		union
		{
			BasicBlock* pBasicBlock; // parent
			Function* pFunction;
			Module* pModule;
		} m_parent = {};

		using DualOpMemberFun = Instruction* (Instruction::*)(Instruction*, Instruction*);

	public:
		using Iterator = EntryIterator<Operand>;

		template <class ...Args>
		Instruction(Module* _pModule, const spv::Op _op = spv::Op::OpNop, Args&& ... _args);
		template <class ...Args>
		Instruction(Function* _pFunction, const spv::Op _op = spv::Op::OpNop, Args&& ... _args);
		template <class ...Args>
		Instruction(BasicBlock* _pBasicBlock, const spv::Op _op = spv::Op::OpNop, Args&& ... _args);

		Instruction(Module* _pModule, Instruction&& _other) noexcept;
		Instruction(Function* _pFunction, Instruction&& _other) noexcept;
		Instruction(BasicBlock* _pBasicBlock, Instruction&& _other) noexcept;

		Instruction(const Instruction&) = delete;

		virtual ~Instruction();

		Instruction& operator=(Instruction&& _other) noexcept;
		Instruction& operator=(const Instruction& _other) = delete;

		Module* getModule() const;
		Function* getFunction() const;
		BasicBlock* getBasicBlock() const;

		// get name that was assigned via opName
		const char* getName() const;

		// not so sure this is a good idea
		operator BasicBlock& () { return *getBasicBlock(); }
		operator const BasicBlock& () const { return *getBasicBlock(); }

		// manual instruction construction:
		void setOperation(const spv::Op _op) { m_Operation = _op; };
		spv::Op getOperation() const { return m_Operation; }
		template<class ...Args>
		Operand& addOperand(Args&& ... _operand) { return emplace_back(stdrep::forward<Args>(_operand)...); }

		// operand helper
		spv::Id getResultId() const;
		Instruction* getTypeInstr() const;
		const Type* getType() const;

		// operand iterator helpers return null iterator of not compatible with operation 
		Iterator getResultTypeOperand() const; // <id> ResultType operand
		Iterator getResultIdOperand() const; // Result <id> operand
		Iterator getFirstActualOperand() const; // operand after Result <id>

		// get StorageClass of OpVariable instructions
		spv::StorageClass getStorageClass() const;
	
		bool isType() const;
		bool isTerminator() const;
		bool isConstant() const;
		bool isSpecConstant() const;
		bool isSpecOrConstant() const;

		bool hasResult() const { return hasResultId(m_Operation); }
		bool hasResultType() const { return hasResultTypeId(m_Operation); }
		bool hasResultAndType() const { return hasResultAndTypeId(m_Operation); }

		// returns a list of BasicBlock branch targets if this instuction is a Terminator
		bool getBranchTargets(List<BasicBlock*>& _outTargetBlocks) const;

		// reset Operation and clear Operands
		void reset();

		// get number of 32 bit words used by this instruction
		unsigned int getWordCount() const;

		// get opcode encoded with instruction word count [16 bit op code, 16 bit number of operand words] 
		unsigned int getOpCode() const;

		// returns the ID assigned to this instrucions
		spv::Id resolveId(spv::Id& _previousId);

		// serialize instructions of this basic block to the IWriter
		void write(IWriter* _pWriter);

		// transforms _args to operands, calls inferResultTypeOperand and validateOperands()
		template <class ...Args>
		Instruction* makeOp(const spv::Op _op, Args ... _args);

		// convert and add the raw data passed via _args as literal_t operands
		template <class ...Args>
		void appendLiterals(Args ... _args);

		// infer result type from operands, RestulType operand must be set to InvalidInstr
		// this function assigns the infered OpType instruction to the first operand of this instruction
		// uses ITypeInferenceAndValiation instance if present
		Instruction* inferResultTypeOperand();

		// validate m_Operations and operands with ITypeInferenceAndValiation instance if present, return true if okay
		bool validateOperands();

		// checks if this instruction is the Modules generic invalid instruction (OpNop)
		bool isErrorInstr() const;

		//
		// GENERIC OPERATIONS
		//

		Instruction* Add(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opIAdd, &Instruction::opFAdd, "Failed to match Add's operand types for this instruction"); }
		Instruction* Sub(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opISub, &Instruction::opFSub, "Failed to match Add's operand types for this instruction"); }
		Instruction* Mul(Instruction* _pLeft, Instruction* _pRight);
		Instruction* Div(Instruction* _pLeft, Instruction* _pRight, bool _allowVecDividedByScalar = true);

		Instruction* Not(Instruction* _pIntOrBool); // lowecase not is a c++ keywork, hence generic functions need to be upper case
		Instruction* Equal(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opIEqual, &Instruction::opFOrdEqual, "Failed to match Equals's operand types for this instruction"); }
		Instruction* NotEqual(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opINotEqual, &Instruction::opFOrdNotEqual, "Failed to match NotEqual's operand types for this instruction"); }
		Instruction* Greater(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opSGreaterThan, &Instruction::opUGreaterThan, &Instruction::opFOrdGreaterThan, "Failed to match Greater's operand types for this instruction"); }
		Instruction* GreaterEqual(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opSGreaterThanEqual, &Instruction::opUGreaterThanEqual, &Instruction::opFOrdGreaterThanEqual, "Failed to match GreaterEqual's operand types for this instruction"); }
		Instruction* Less(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opSLessThan, &Instruction::opULessThan, &Instruction::opFOrdLessThan, "Failed to match Less's operand types for this instruction"); }
		Instruction* LessEqual(Instruction* _pLeft, Instruction* _pRight) { return intFloatOp(_pLeft, _pRight, &Instruction::opSLessThanEqual, &Instruction::opULessThanEqual, &Instruction::opFOrdLessThanEqual, "Failed to match LessEqual's operand types for this instruction"); }

		//
		// SPIR-V OPERATIONS
		//

		void opNop();

		Instruction* opUndef(Instruction* _pResultType);

		Instruction* opSizeOf(Instruction* _pPointerToVar);

		// instruction generators:
		// all instructions generating a result id return a pointer to this instruction for reference (passing to other instruction operand)
		void opCapability(const spv::Capability _capability);

		void opMemoryModel(const spv::AddressingModel _addressModel, const spv::MemoryModel _memoryModel);

		void opExtension(const char* _pExtName);

		// generates extension id
		Instruction* opExtInstImport(const char* _pExtName);

		template <class ...Operands>
		Instruction* opExtInst(Instruction* _pResultType, Instruction* _pExtensionId, unsigned int _instOpCode, Operands ... _operands);

		template <class ...Operands>
		Instruction* opExtInst(Instruction* _pResultType, const char* _pExtName, unsigned int _instOpCode, Operands ... _operands);

		Instruction* opLabel();

		Instruction* opFunction(const Flag<spv::FunctionControlMask> _functionControl, Instruction* _pResultType, Instruction* _pFuncType);

		Instruction* opFunctionParameter(Instruction* _pType);

		void opReturn();

		void opReturnValue(Instruction* _pValue);

		void opFunctionEnd();

		template <class ... ArgInstr>
		Instruction* opFunctionCall(Instruction* _pResultType, Instruction* _pFunction, ArgInstr ... _args);

		template <class ... ArgInstr>
		Instruction* call(Function* _pFunction, ArgInstr ... _args);

		//  _pFunction is result of opFunction
		template <class ... Instr>
		void opEntryPoint(const spv::ExecutionModel _model, Instruction* _pFunction, const char* _pName, Instr ... _instr);

		// _pResultType must be of OpTypePointer
		Instruction* opVariable(Instruction* _pResultType, const spv::StorageClass _storageClass, Instruction* _pInitializer = nullptr);

		void opName(Instruction* _pTarget, const char* _pName);

		void opMemberName(Instruction* _pTargetStructType, unsigned int _memberIndex, const char* _pName);

		template <class ... Decorations>
		void opDecorate(Instruction* _pTarget, spv::Decoration _decoration, Decorations ... _decorations);

		template <class ... Decorations>
		void opMemberDecorate(Instruction* _pTargetStructType, unsigned int _memberIndex, spv::Decoration _decoration, Decorations ... _decorations);

		template <class ... Instr>
		void opMemberId(Instruction* _pTarget, spv::Decoration _decoration, Instruction* _pId, Instr*..._ids);

		// deduce parent form input variables
		template <class ... VarInst>
		Instruction* opPhi(Instruction* _pVar, VarInst* ... _variables);

		Instruction* opSelect(Instruction* _pCondBool, Instruction* _pTrueObj, Instruction* _pFalseObj);

		template <class ...LoopControlParams>
		void opLoopMerge(BasicBlock* _pMergeBlock, BasicBlock* _pContinueBlock, const Flag<spv::LoopControlMask> _loopControl, LoopControlParams ... _params);

		void opSelectionMerge(BasicBlock* _pMergeBlock, const spv::SelectionControlMask _control);

		// label is infered from the basic block on serialization
		void opBranch(BasicBlock* _pTargetBlock);

		// label is infered from the basic block on serialization
		void opBranchConditional(Instruction* _pCondition, BasicBlock* _pTrueBlock, BasicBlock* _pFalseBlock);
		void opBranchConditional(Instruction* _pCondition, BasicBlock* _pTrueBlock, BasicBlock* _pFalseBlock, const unsigned int _trueWeight, const unsigned int _falseWeight);

		template <class ... Instr>
		Instruction* opAccessChain(Instruction* _pResultType, Instruction* _pBase, Instruction* _pConstIndex, Instr* ... _pIndices);

		template <class ... IntIndices>
		Instruction* opAccessChain(Instruction* _pBase, const unsigned int _firstIndex, IntIndices... _indices);

		template <class ... Instr>
		Instruction* opInBoundsAccessChain(Instruction* _pResultType, Instruction* _pBase, Instruction* _pConstIndex, Instr* ... _pIndices);

		template <class ... Operands>
		Instruction* opLoad(Instruction* _pPointerToVar, const Flag<MemoryOperands> _memOperands = MemoryOperands::None, Operands ... _operands);

		template <class ... Operands>
		void opStore(Instruction* _pPointerToVar, Instruction* _valueToStore, const Flag<MemoryOperands> _memOperands = MemoryOperands::None, Operands ... _operands);

		Instruction* opOuterProduct(Instruction* _pLeft, Instruction* _pRight);

		Instruction* opDot(Instruction* _pLeft, Instruction* _pRight);

		Instruction* opAny(Instruction* _pBoolVec);

		Instruction* opAll(Instruction* _pBoolVec);

		Instruction* opNot(Instruction* _pIntVec) { return scalarVecOp(spv::Op::OpNot, _pIntVec, nullptr, "Operand of OpNot is not a scalar or vector of int type"); }

		Instruction* opLogicalNot(Instruction* _pBoolVec) { return scalarVecOp(spv::Op::OpLogicalNot, _pBoolVec, nullptr, "Operand of OpLogicalNot is not a scalar or vector of bool type"); }

		Instruction* opIEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpIEqual, _pLeft, _pRight, "Operand of OpIEqual is not a scalar or vector of int type"); }

		Instruction* opINotEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpINotEqual, _pLeft, _pRight, "Operand of OpINotEqual is not a scalar or vector of int type"); }

		Instruction* opUGreaterThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpUGreaterThan, _pLeft, _pRight, "Operand of OpUGreaterThan is not a scalar or vector of int type", false); }

		Instruction* opUGreaterThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpUGreaterThanEqual, _pLeft, _pRight, "Operand of OpUGreaterThanEqual is not a scalar or vector of int type", false); }

		Instruction* opSGreaterThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSGreaterThan, _pLeft, _pRight, "Operand of OpSGreaterThan is not a scalar or vector of int type", false); }

		Instruction* opSGreaterThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSGreaterThanEqual, _pLeft, _pRight, "Operand of OpSGreaterThanEqual is not a scalar or vector of int type", false); }

		Instruction* opULessThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpULessThan, _pLeft, _pRight, "Operand of OpULessThan is not a scalar or vector of int type", false); }

		Instruction* opULessThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpULessThanEqual, _pLeft, _pRight, "Operand of OpULessThanEqual is not a scalar or vector of int type", false); }

		Instruction* opSLessThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSLessThan, _pLeft, _pRight, "Operand of OpSLessThan is not a scalar or vector of int type", false); }

		Instruction* opSLessThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSLessThanEqual, _pLeft, _pRight, "Operand of OpSLessThanEqual is not a scalar or vector of int type", false); }

		Instruction* opFOrdEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdEqual, _pLeft, _pRight, "Operand of OpFOrdEqual is not a scalar or vector of float type"); }

		Instruction* opFUnordEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordEqual, _pLeft, _pRight, "Operand of OpFUnordEqual is not a scalar or vector of float type"); }

		Instruction* opFOrdNotEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdNotEqual, _pLeft, _pRight, "Operand of OpFOrdNotEqual is not a scalar or vector of float type"); }

		Instruction* opFUnordNotEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordNotEqual, _pLeft, _pRight, "Operand of OpFUnordNotEqual is not a scalar or vector of float type"); }

		Instruction* opFOrdLessThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdLessThan, _pLeft, _pRight, "Operand of OpFOrdLessThan is not a scalar or vector of float type"); }

		Instruction* opFUnordLessThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordLessThan, _pLeft, _pRight, "Operand of OpFUnordLessThan is not a scalar or vector of float type"); }

		Instruction* opFOrdGreaterThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdGreaterThan, _pLeft, _pRight, "Operand of OpFOrdGreaterThan is not a scalar or vector of float type"); }

		Instruction* opFUnordGreaterThan(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordGreaterThan, _pLeft, _pRight, "Operand of OpFUnordGreaterThan is not a scalar or vector of float type"); }

		Instruction* opFOrdLessThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdLessThanEqual, _pLeft, _pRight, "Operand of OpFOrdLessThanEqual is not a scalar or vector of float type"); }

		Instruction* opFUnordLessThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordLessThanEqual, _pLeft, _pRight, "Operand of OpFUnordLessThanEqual is not a scalar or vector of float type"); }

		Instruction* opFOrdGreaterThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFOrdGreaterThanEqual, _pLeft, _pRight, "Operand of OpFOrdGreaterThanEqual is not a scalar or vector of float type"); }

		Instruction* opFUnordGreaterThanEqual(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFUnordGreaterThanEqual, _pLeft, _pRight, "Operand of OpFUnordGreaterThanEqual is not a scalar or vector of float type"); }

		Instruction* opVectorExtractDynamic(Instruction* _pVector, Instruction* _pIndexInt);

		Instruction* opVectorInsertDynamic(Instruction* _pVector, Instruction* _pComponent, Instruction* _pIndexInt);

		template <class ... ConstituentInstr>
		Instruction* opCompositeConstruct(Instruction* _pResultType, Instruction* _pFirstConstituents, ConstituentInstr* ... _constituents);

		template <class ... IntIndices>
		Instruction* opCompositeExtract(Instruction* _pComposite, const unsigned int _firstIndex, IntIndices ... _indices);

		template <class ... IntIndices>
		Instruction* opCompositeInsert(Instruction* _pComposite,  Instruction* _pValue, const unsigned int _firstIndex, IntIndices ... _indices);

		Instruction* opCopyObject(Instruction* _pObject);

		Instruction* opTranspose(Instruction* _pMatrix);

		Instruction* opSNegate(Instruction* _pSignedInt) { return scalarVecOp(spv::Op::OpSNegate, _pSignedInt, nullptr, "Operand of OpSNegate is not a scalar or vector of int type"); }

		Instruction* opFNegate(Instruction* _pFloat) { return scalarVecOp(spv::Op::OpFNegate, _pFloat, nullptr, "Operand of OpFNegate is not a scalar or vector of float type"); }

		Instruction* opIAdd(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpIAdd, _pLeft, _pRight, "Operand of opIAdd is not a scalar or vector of int type"); }

		Instruction* opFAdd(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFAdd, _pLeft, _pRight, "Operand of OpFAdd is not a scalar or vector of float type"); }

		Instruction* opISub(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpISub, _pLeft, _pRight, "Operand of OpISub is not a scalar or vector of int type"); }

		Instruction* opFSub(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFSub, _pLeft, _pRight, "Operand of OpFSub is not a scalar or vector of float type"); }

		Instruction* opIMul(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpIMul, _pLeft, _pRight, "Operand of OpIMul is not a scalar or vector of int type"); }

		Instruction* opFMul(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFMul, _pLeft, _pRight, "Operand of OpFMul is not a scalar or vector of float type"); }
		
		Instruction* opUDiv(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpUDiv, _pLeft, _pRight, "Operand of OpUDiv is not a scalar or vector of unsigned int type"); }

		Instruction* opSDiv(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSDiv, _pLeft, _pRight, "Operand of OpSDiv is not a scalar or vector of signed int type", false); }

		Instruction* opFDiv(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFDiv, _pLeft, _pRight, "Operand of OpFDiv is not a scalar or vector of float type"); }
		
		Instruction* opUMod(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpUMod, _pLeft, _pRight, "Operand of OpUMod is not a scalar or vector of unsigned int type"); }

		Instruction* opSMod(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSMod, _pLeft, _pRight, "Operand of OpSMod is not a scalar or vector of int type", false); }

		Instruction* opFMod(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFMod, _pLeft, _pRight, "Operand of OpFMod is not a scalar or vector of float type"); }

		Instruction* opSRem(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpSRem, _pLeft, _pRight, "Operand of OpSRem is not a scalar or vector of int type", false); }
		
		Instruction* opFRem(Instruction* _pLeft, Instruction* _pRight) { return scalarVecOp(spv::Op::OpFRem, _pLeft, _pRight, "Operand of OpFRem is not a scalar or vector of float type"); }
		
		Instruction* opVectorTimesScalar(Instruction* _pVector, Instruction* _pScalar);

		Instruction* opMatrixTimesScalar(Instruction* _pMatrix, Instruction* _pScalar);

		Instruction* opVectorTimesMatrix(Instruction* _pVector, Instruction* _pMatrix);

		Instruction* opMatrixTimesVector(Instruction* _pMatrix, Instruction* _pVector);

		Instruction* opMatrixTimesMatrix(Instruction* _pLeft, Instruction* _pRight);

		Instruction* opSampledImage(Instruction* _pImage, Instruction* _pSampler);

		// generic base case with image operands
		template <class ...ImageOperands>
		Instruction* opImageSample(const spv::Op _imageSampleOp, Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDrefOrCompnent, const Flag<spv::ImageOperandsMask> _imageOperands, ImageOperands... _operands);

#pragma region SampleMethods
		// Implicit
		template <class ...ImageOperands>
		Instruction* opImageSampleImplictLod(Instruction* _pSampledImage, Instruction* _pCoordinate)
		{
			return opImageSample(spv::Op::OpImageSampleImplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::MaskNone);
		}

		// Explicit
		template <class ...ImageOperands>
		Instruction* opImageSampleExplicitLodLevel(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pLodLevel)
		{
			return opImageSample(spv::Op::OpImageSampleExplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::Lod, _pLodLevel);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleExplicitLodGrad(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pGrad)
		{
			return opImageSample(spv::Op::OpImageSampleExplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::Grad, _pGrad);
		}

		// Proj
		template <class ...ImageOperands>
		Instruction* opImageSampleProjImplictLod(Instruction* _pSampledImage, Instruction* _pCoordinate)
		{
			return opImageSample(spv::Op::OpImageSampleProjImplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::MaskNone);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleProjExplicitLodLevel(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pLodLevel)
		{
			return opImageSample(spv::Op::OpImageSampleProjExplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::Lod, _pLodLevel);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleProjExplicitLodGrad(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pGrad)
		{
			return opImageSample(spv::Op::OpImageSampleProjExplicitLod, _pSampledImage, _pCoordinate, nullptr, spv::ImageOperandsMask::Grad, _pGrad);
		}

		//
		// Dref
		//

		// Implicit
		template <class ...ImageOperands>
		Instruction* opImageSampleDrefImplictLod(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference)
		{
			return opImageSample(spv::Op::OpImageSampleDrefImplicitLod, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::MaskNone);
		}

		// Explicit
		template <class ...ImageOperands>
		Instruction* opImageSampleDrefExplicitLodLevel(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference, Instruction* _pLodLevel)
		{
			return opImageSample(spv::Op::OpImageSampleDrefExplicitLod, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::Lod, _pLodLevel);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleDrefExplicitLodGrad(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference, Instruction* _pGrad)
		{
			return opImageSample(spv::Op::OpImageSampleDrefExplicitLod, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::Grad, _pGrad);
		}

		// Proj
		template <class ...ImageOperands>
		Instruction* opImageSampleProjDrefImplictLod(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference)
		{
			return opImageSample(spv::Op::OpImageSampleProjDrefImplicitLod, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::MaskNone);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleProjDrefExplicitLodLevel(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference, Instruction* _pLodLevel)
		{
			return opImageSample(spv::Op::OpImageSampleProjDrefExplicitLod, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::Lod, _pLodLevel);
		}

		template <class ...ImageOperands>
		Instruction* opImageSampleProjDrefExplicitLodGrad(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference, Instruction* _pGrad)
		{
			return opImageSample(spv::Op::OpImageSampleProjDrefExplicitLod, _pSampledImage, _pCoordinate, _pDepthReference spv::ImageOperandsMask::Grad, _pGrad);
		}

		template <class ...ImageOperands>
		Instruction* opImageFetch(Instruction* _pImage, Instruction* _pCoordinate)
		{
			return opImageSample(spv::Op::OpImageFetch, _pImage, _pCoordinate, nullptr, spv::ImageOperandsMask::MaskNone);
		}

		// Gather
		template <class ...ImageOperands>
		Instruction* opImageGather(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pComponent)
		{
			return opImageSample(spv::Op::OpImageGather, _pSampledImage, _pCoordinate, _pComponent, spv::ImageOperandsMask::MaskNone);
		}

		template <class ...ImageOperands>
		Instruction* opImageDrefGather(Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDepthReference)
		{
			return opImageSample(spv::Op::OpImageDrefGather, _pSampledImage, _pCoordinate, _pDepthReference, spv::ImageOperandsMask::MaskNone);
		}

#pragma endregion

		Instruction* opConvertFToU(Instruction* _pFloatVec) { return scalarVecOp(spv::Op::OpConvertFToU, _pFloatVec, nullptr, "Operand of OpConvertFToU is not a scalar or vector of float type", false); }

		Instruction* opConvertFToS(Instruction* _pFloatVec) { return scalarVecOp(spv::Op::OpConvertFToS, _pFloatVec, nullptr, "Operand of OpConvertFToS is not a scalar or vector of float type", false); }

		Instruction* opConvertSToF(Instruction* _pSIntVec) { return scalarVecOp(spv::Op::OpConvertSToF, _pSIntVec, nullptr, "Operand of OpConvertSToF is not a scalar or vector of int type"); }

		Instruction* opConvertUToF(Instruction* _pUIntVec) { return scalarVecOp(spv::Op::OpConvertUToF, _pUIntVec, nullptr, "Operand of OpConvertUToF is not a scalar or vector of int type"); }

		// convert scalar or vector to different componenet bit width
		Instruction* opUConvert(Instruction* _pUIntVec, unsigned int _bitWidth);

		Instruction* opSConvert(Instruction* _pSIntVec, unsigned int _bitWidth);

		Instruction* opFConvert(Instruction* _pFloatVec, unsigned int _bitWidth);

		Instruction* opQuantizeToF16(Instruction* _pFloatVec);

		Instruction* opConvertPtrToU(Instruction* _pPhysPtr, unsigned int _bitWidth);

		Instruction* opBitcast(Instruction* _pResultType, Instruction* _pOperand);

		template<class T> // generic version of opBitcast, generates spv type from T
		Instruction* bitcast(Instruction* _pOperand);

		template <class ...Args>
		Instruction* opSpecConstantOp(Instruction* _pResultType, spv::Op _opcode, Args&& ..._instrOperands);

		// convert this instruction to its OpSpecConstantOp variant
		Instruction* toSpecOp();

	private:
		// return error instr
		Instruction* error() const;

		// creates literals
		template <class T, class ...Args>
		void makeOpInternal(T first, Args ... _args);

		// checks types based on passed _type and_sign
		Instruction* scalarVecOp(spv::Op _op, spv::Op _type, Sign _sign, Instruction* _pLeft, Instruction* _pRight, const char* _pErrorMsg, bool _checkSign);

		// checks types based on passed _op using getTypeFromOp(_op, sign)
		Instruction* scalarVecOp(spv::Op _op, Instruction* _pLeft, Instruction* _pRight = nullptr, const char* _pErrorMsg = nullptr, bool _checkSign = true);

		// decides based on type of _pLeft and _pRight if _intFun or _floatFun should be called
		Instruction* intFloatOp(Instruction* _pLeft, Instruction* _pRight, DualOpMemberFun _intFun, DualOpMemberFun _floatFun, const char* _pErrorMsg = nullptr);
		
		// decides based on type of _pLeft and _pRight if signed _sIntFun, unsigned _uIntFund or float _floatFun should be called
		// unsigned & unsinged => unsigned, signed & unsigned => signed, float & float => float
		Instruction* intFloatOp(Instruction* _pLeft, Instruction* _pRight, DualOpMemberFun _sIntFun, DualOpMemberFun _uIntFun, DualOpMemberFun _floatFun, const char* _pErrorMsg = nullptr);
	};

	template<class ...Args>
	inline Instruction::Instruction(Module* _pModule, const spv::Op _op, Args&& ..._args) :List(_pModule->getAllocator()),
		m_parentType(ParentType::Module)
	{
		m_parent.pModule = _pModule;
		makeOp(_op, stdrep::forward<Args>(_args)...);
	}

	template<class ...Args>
	inline Instruction::Instruction(Function* _pFunction, const spv::Op _op, Args&& ..._args) : List(_pFunction->getAllocator()),
		m_parentType(ParentType::Function)
	{
		m_parent.pFunction = _pFunction;
		makeOp(_op, stdrep::forward<Args>(_args)...);
	}

	template<class ...Args>
	inline Instruction::Instruction(BasicBlock* _pBasicBlock, const spv::Op _op, Args&& ..._args) : List(_pBasicBlock->getAllocator()),
		m_parentType(ParentType::BasicBlock)
	{
		m_parent.pBasicBlock = _pBasicBlock;
		makeOp(_op, stdrep::forward<Args>(_args)...);
	}

	template<class ...Args>
	inline Instruction* Instruction::makeOp(const spv::Op _op, Args ..._args)
	{
		reset();

		m_Operation = _op;

		if constexpr (sizeof...(_args) > 0u)
		{
			makeOpInternal(_args...);
		}

		inferResultTypeOperand();
		validateOperands();

		return this;
	}
	
	template<class T>
	inline Instruction* Instruction::bitcast(Instruction* _pOperand)
	{
		return opBitcast(getModule()->type<T>(), _pOperand);
	}

	template<class ...Args>
	inline Instruction* Instruction::opSpecConstantOp(Instruction* _pResultType, spv::Op _opcode, Args&& ..._instrOperands)
	{
		// TODO: check _opcode for valid operations
		return makeOp(spv::Op::OpSpecConstantOp, _pResultType, InvalidId, _opcode, stdrep::forward<Args>(_instrOperands)...);
	}

	template<class T, class ...Args>
	inline void Instruction::makeOpInternal(T _first, Args ..._args)
	{
		if constexpr (traits::is_same_base_type_v<T, Instruction*> || traits::is_same_base_type_v<T, BasicBlock*> || traits::is_same_base_type_v<T, spv::Id> || traits::is_same_base_type_v<T, literal_t>)
		{
			addOperand(_first);
		}
		else if constexpr (stdrep::is_same_v<traits::remove_cvref_t<T>, int> || stdrep::is_same_v<traits::remove_cvref_t<T>, unsigned int>) // bitcast to 32 bit literal
		{
			addOperand(literal_t{_first});
		}
		else
		{
			appendLiterals(_first);
		}

		if constexpr (sizeof...(_args) > 0u)
		{
			makeOpInternal(_args...);
		}
	}

	template<class ...Args>
	inline void Instruction::appendLiterals(Args ..._args)
	{
		appendLiteralsToContainer(*this, _args...);
	}

	template<class ...Operands>
	inline Instruction* Instruction::opExtInst(Instruction* _pResultType, Instruction* _pExtensionId, unsigned int _instOpCode, Operands ..._operands)
	{
		return makeOp(spv::Op::OpExtInst, _pResultType, InvalidId, _pExtensionId, literal_t{ _instOpCode }, _operands...);
	}

	template<class ...Operands>
	inline Instruction* Instruction::opExtInst(Instruction* _pResultType, const char* _pExtName, unsigned int _instOpCode, Operands ..._operands)
	{
		Instruction* _pExtImport = getModule()->getExtensionInstructionImport(_pExtName);
		return makeOp(spv::Op::OpExtInst, _pResultType, InvalidId, _pExtImport, literal_t{ _instOpCode }, _operands...);
	}

	template<class ...ArgInstr>
	inline Instruction* Instruction::opFunctionCall(Instruction* _pResultType, Instruction* _pFunction, ArgInstr ..._args)
	{
		return makeOp(spv::Op::OpFunctionCall, _pResultType, InvalidId, _pFunction, _args...);
	}

	template<class ...ArgInstr>
	inline Instruction* Instruction::call(Function* _pFunction, ArgInstr ..._args)
	{
		return opFunctionCall(_pFunction->getReturnType(), _pFunction->getFunction(), _args...);
	}

	template<class ...Instr>
	inline void Instruction::opEntryPoint(const spv::ExecutionModel _model, Instruction* _pFunction, const char* _pName, Instr ..._instr)
	{
		makeOp(spv::Op::OpEntryPoint, _model, _pFunction, _pName, _instr...);
	}

	template<class ...Decorations>
	inline void Instruction::opDecorate(Instruction* _pTarget, spv::Decoration _decoration, Decorations ..._decorations)
	{
		makeOp(spv::Op::OpDecorate, _pTarget, _decoration, _decorations...);
	}

	template<class ...Decorations>
	inline void Instruction::opMemberDecorate(Instruction* _pTargetStructType, unsigned int _memberIndex, spv::Decoration _decoration, Decorations ..._decorations)
	{
		makeOp(spv::Op::OpMemberDecorate, _pTargetStructType, _memberIndex, _decoration, _decorations...);
	}

	template<class ...Instr>
	inline void Instruction::opMemberId(Instruction* _pTarget, spv::Decoration _decoration, Instruction* _pId, Instr* ..._ids)
	{
		makeOp(spv::Op::OpDecorateId, _pTarget, _decoration, _ids...);
	}

	template<class ...VarInst>
	inline Instruction* Instruction::opPhi(Instruction* _pVar, VarInst* ..._variables)
	{
		makeOp(spv::Op::OpPhi, _pVar->getTypeInstr(), InvalidId);

		auto addVar = [&](Instruction* var)
		{
			addOperand(var);
			addOperand(var->getBasicBlock());
		};

		addVar(_pVar);

		(addVar(_variables), ...);
		return this;
	}

	template<class ...LoopControlParams>
	inline void Instruction::opLoopMerge(BasicBlock* _pMergeBlock, BasicBlock* _pContinueBlock, const Flag<spv::LoopControlMask> _loopControl, LoopControlParams ..._params)
	{
		makeOp(spv::Op::OpLoopMerge, _pMergeBlock, _pContinueBlock, literal_t{ _loopControl }, _params...);
	}

	template<class ...Instr>
	inline Instruction* Instruction::opAccessChain(Instruction* _pResultType, Instruction* _pBase, Instruction* _pConstIndex, Instr* ..._pIndices)
	{
		return makeOp(spv::Op::OpAccessChain, _pResultType, InvalidId, _pBase, _pConstIndex, _pIndices...);
	}

	template<class ...IntIndices>
	inline Instruction* Instruction::opAccessChain(Instruction* _pBase, const unsigned int _firstIndex, IntIndices ..._indices)
	{
		// Base must be a pointer, pointing to the base of a composite object.

		const Type* pBaseType = _pBase->getType();
		if (pBaseType == nullptr) return this;

		auto it = pBaseType->getSubType(0u, _firstIndex, _indices...); // base is a pointer type, so 0 is used to get the inner type
		Module* pModule = _pBase->getModule();
		Instruction* pResultType = nullptr;

		if (it != nullptr)
		{
			// Result Type must be an OpTypePointer.
			// Its Type operand must be the type reached by walking the Base’s type hierarchy down to the last provided index in Indexes, and its Storage Class operand must be the same as the Storage Class of Base.
	
			Type&& ptrType(it->wrap(spv::Op::OpTypePointer));
			ptrType.setStorageClass(pBaseType->getStorageClass());
			pResultType = pModule->addType(ptrType);
		}

		return makeOp(spv::Op::OpAccessChain, pResultType, InvalidId, _pBase, pModule->constant(_firstIndex), pModule->constant<unsigned int>(_indices)...);
	}

	template<class ...Instr>
	inline Instruction* Instruction::opInBoundsAccessChain(Instruction* _pResultType, Instruction* _pBase, Instruction* _pConstIndex, Instr* ..._pIndices)
	{
		return makeOp(spv::Op::OpInBoundsAccessChain, _pResultType, InvalidId, _pBase, _pConstIndex, _pIndices...);
	}

	template<class ...Operands>
	inline Instruction* Instruction::opLoad(Instruction* _pPointerToVar, const Flag<MemoryOperands> _memOperands, Operands ..._operands)
	{
		const Type* ptrType = _pPointerToVar->getType();
		if (ptrType == nullptr) return this;

		// Result Type is the type of the loaded object.It must be a type with ﬁxed size; i.e., it cannot be, nor include, any OpTypeRuntimeArray types.
		// Pointer is the pointer to load through.Its type must be an OpTypePointer whose Type operand is the same as Result Type.
		Instruction* pResultType = getModule()->addType(ptrType->front());
		return makeOp(spv::Op::OpLoad, pResultType, InvalidId, _pPointerToVar, _memOperands.mask, _operands...);
	}

	template<class ...Operands>
	inline void Instruction::opStore(Instruction* _pPointerToVar, Instruction* _valueToStore, const Flag<MemoryOperands> _memOperands, Operands ..._operands)
	{
		makeOp(spv::Op::OpStore, _pPointerToVar, _valueToStore, _memOperands.mask, _operands...);
	}

	template<class ...ConstituentInstr>
	inline Instruction* Instruction::opCompositeConstruct(Instruction* _pResultType, Instruction* _pFirstConstituent, ConstituentInstr* ..._constituents)
	{
		const Type* type =  _pResultType->getType();
		if (type == nullptr) return this;

		if (type->isComposite() == false)
		{
			getModule()->logError("Result type of opCompositeConstruct is not a composite type");
			return this;
		}

		return makeOp(spv::Op::OpCompositeConstruct, _pResultType, InvalidId, _pFirstConstituent, _constituents...);
	}

	template<class ...IntIndices>
	inline Instruction* Instruction::opCompositeExtract(Instruction* _pComposite, const unsigned int _firstIndex, IntIndices ..._indices)
	{
		const Type* pBaseType = _pComposite->getType();
		if (pBaseType == nullptr) return this;

		Module* pModule = _pComposite->getModule();

		if (pBaseType->isComposite() == false)
		{
			pModule->logError("Argument of opCompositeExtract is not a composite type");
			return this;
		}

		auto it = pBaseType->getSubType(0u, _firstIndex, _indices...);

		if (it != nullptr)
		{
			Instruction* pResultType = pModule->addType(*it);
			return makeOp(spv::Op::OpCompositeExtract, pResultType, InvalidId, _pComposite, literal_t{ _firstIndex }, literal_t{ _indices }...);
		}

		pModule->logError("Invalid index sequence specified for composite type extraction");

		return this;
	}

	template<class ...IntIndices>
	inline Instruction* Instruction::opCompositeInsert(Instruction* _pComposite, Instruction* _pValue, const unsigned int _firstIndex, IntIndices ..._indices)
	{
		const Type* pBaseType = _pComposite->getType();
		if (pBaseType == nullptr) return this;

		Module* pModule = _pComposite->getModule();

		if (pBaseType->isComposite() == false)
		{
			pModule->logError("Argument of opCompositeInsert is not a composite type");
			return this;
		}

		auto it = pBaseType->getSubType(0u, _firstIndex, _indices...);

		if (it != nullptr)
		{
			const Type* pValueType = _pValue->getType();
			if (pValueType == nullptr) return this;

			if (*it == *pValueType)
			{
				return makeOp(spv::Op::OpCompositeInsert, _pComposite->getTypeInstr(), InvalidId, _pValue, _pComposite, literal_t{ _firstIndex }, literal_t{ _indices }...);
			}

			pModule->logError("Value type does not match composite insertion type");
			return this;
		}

		pModule->logError("Invalid index sequence specified for composite insertion");

		return this;
	}

	template<class ...ImageOperands>
	inline Instruction* Instruction::opImageSample(const spv::Op _imageSampleOp, Instruction* _pSampledImage, Instruction* _pCoordinate, Instruction* _pDrefOrCompnent, const Flag<spv::ImageOperandsMask> _imageOperands, ImageOperands ..._operands)
	{
		Module& module = *getModule();
		const Type* type = _pSampledImage->getType();
		const Type* imageType = type->isSampledImage() ? &type->front() : type;
		const Type* coordType = _pCoordinate->getType();

		if (type == nullptr || imageType == nullptr || coordType == nullptr) return this;

		bool isDref = false;
		bool isProj = false;
		bool isComponent = false;
		bool coordCanBeInt = false;
		bool checkCoords = false;

		auto checkCoordinateType = [&]() -> bool
		{
			if (checkCoords == false)
				return true;

			unsigned int dim = 0u;

			switch (imageType->getImageDimension())
			{
			case spv::Dim::Dim1D:
			case spv::Dim::Buffer:
				dim = 1u;
				break;
			case spv::Dim::Dim2D:
			case spv::Dim::Rect:
			case spv::Dim::SubpassData:
				dim = 2u;
				break;
			case spv::Dim::Dim3D:
			case spv::Dim::Cube:
				dim = 3u;
				break;
			default:
				module.logError("Image dimension not supported/implemented");
				return false;
			}

			if (imageType->getImageArray())
			{
				dim += 1u;
			}

			if (isProj)
			{
				dim += 1u;
			}

			if (dim > 1u && coordType->getVectorComponentCount() < dim)
			{
				module.logError("Dimension of coordinates does not match image type");
				return false;
			}

			if (coordType->isScalarOrVectorOf(spv::Op::OpTypeInt))
			{
				if (coordCanBeInt == false)
				{
					module.logError("Image operation does not support integer coordinates");
					return false;
				}
			}
			else if (coordType->isScalarOrVectorOf(spv::Op::OpTypeFloat) == false)
			{
				module.logError("Coordinate type must be scalar or vector of float");
				return false;
			}

			return true;
		};

		auto checkDrefComponent = [&]() -> bool
		{
			if (isDref || isComponent)
			{
				if (_pDrefOrCompnent == nullptr || _pDrefOrCompnent->getType() == nullptr)
				{
					module.logError("Depth reference parameter is null");
					return false;
				}
				if (isDref && _pDrefOrCompnent->getType()->isF32() == false)
				{
					module.logError("Depth reference value must be of type float (32bit)");
					return false;
				}
				if (isComponent && _pDrefOrCompnent->getType()->isI32() == false)
				{
					module.logError("Component index must be of type integer (32bit)");
					return false;
				}
			}
			else if (_pDrefOrCompnent != nullptr)
			{
				module.logError("Image operation does not consume Component or Depth reference value, but operand was supplied");
				return false;
			}
			return true;
		};

		switch (_imageSampleOp)
		{
		case spv::Op::OpImageFetch:
			if (type->isImage() == false || (imageType->getImageSamplerAccess() != SamplerImageAccess::Sampled) || (imageType->getImageDimension() == spv::Dim::Cube))
			{
				module.logError("OpImageFetch requires _pSampledImage of type opImage. Its Dim operand cannot be Cube, and its Sampled operand must be 1.");
				return this;
			}
			checkCoords = true; coordCanBeInt = true;
			break;
		case spv::Op::OpImageDrefGather:
		case spv::Op::OpImageGather:
			isComponent = _imageSampleOp == spv::Op::OpImageGather;
			isDref = _imageSampleOp == spv::Op::OpImageDrefGather;
			checkCoords = true;  coordCanBeInt = true;
			if (imageType->getImageDimension() != spv::Dim::Dim2D && imageType->getImageDimension() != spv::Dim::Cube && imageType->getImageDimension() != spv::Dim::Rect)
			{
				module.logError("OpImageGather requres Dim of sampled image to be 2D, Cube or Rect");
				return this;
			}
			break;
		case spv::Op::OpImageSampleImplicitLod: checkCoords = true; break;
		case spv::Op::OpImageSampleExplicitLod: checkCoords = true; coordCanBeInt = true; break;
		case spv::Op::OpImageSampleProjImplicitLod: isProj = true; checkCoords = true; coordCanBeInt = true; break;
		case spv::Op::OpImageSampleProjExplicitLod: isProj = true; checkCoords = true; break;
		case spv::Op::OpImageSampleDrefImplicitLod: checkCoords = true; isDref = true; break;
		case spv::Op::OpImageSampleDrefExplicitLod: checkCoords = true;  isDref = true; break;
		case spv::Op::OpImageSampleProjDrefImplicitLod: isProj = true; checkCoords = true; isDref = true; break;
		case spv::Op::OpImageSampleProjDrefExplicitLod: isProj = true; checkCoords = true; isDref = true; break;

			break;
		default:
			module.logError("_imageSampleOp is not supported / implemented");
			return this;
		}

		if (!checkDrefComponent() || !checkCoordinateType())
		{
			return this;
		}

		if (_pDrefOrCompnent == nullptr)
		{
			if (_imageOperands == spv::ImageOperandsMask::MaskNone)
			{
				makeOp(_imageSampleOp, InvalidInstr, InvalidId, _pSampledImage, _pCoordinate);			
			}
			else
			{
				makeOp(_imageSampleOp, InvalidInstr, InvalidId, _pSampledImage, _pCoordinate, literal_t{ _imageOperands.mask }, _operands...);
			}
		}
		else
		{
			if (_imageOperands == spv::ImageOperandsMask::MaskNone)
			{
				makeOp(_imageSampleOp, InvalidInstr, InvalidId, _pSampledImage, _pCoordinate, _pDrefOrCompnent);
			}
			else
			{
				makeOp(_imageSampleOp, InvalidInstr, InvalidId, _pSampledImage, _pCoordinate, _pDrefOrCompnent, literal_t{ _imageOperands.mask }, _operands...);
			}
		}

		return this;
	}
} // !spvgentwo