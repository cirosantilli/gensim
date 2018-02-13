/*
 * InterpreterNodeWalker.cpp
 *
 *  Created on: 24 Mar 2015
 *      Author: harry
 */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/GenCInterpreter/InterpreterNodeWalker.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"
#include "genC/ssa/SSAWalkerFactory.h"
#include "genC/ir/IRAction.h"
#include "genC/Parser.h"
#include "genC/ir/IREnums.h"

#include <typeindex>

#define INSTRUMENT_FLOATS 1

namespace gensim
{

	namespace GenCInterpreter
	{
		using namespace genc::ssa;

		class SSAGenCWalker : public SSANodeWalker
		{
		public:

			SSAGenCWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSANodeWalker(_statement, _factory)
			{
			}

			virtual bool EmitDynamicCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const override
			{
				return EmitFixedCode(output, end_label, fully_fixed);
			}

			virtual std::string GetDynamicValue() const override
			{
				return GetFixedValue();
			}

		};

		class SSAAllocRegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSAAllocRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}
			// Don't need to do anything here

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}
		};

		class SSABinaryArithmeticStatementWalker : public SSAGenCWalker
		{
		public:

			SSABinaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			std::string GetFixedValue() const
			{
				//		std::stringstream output;
				//		const SSABinaryArithmeticStatement &stmt = (const SSABinaryArithmeticStatement &)Statement;
				//
				//		output << "(" << Factory.GetOrCreate(stmt.LHS)->GetFixedValue() << genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << Factory.GetOrCreate(stmt.RHS)->GetFixedValue() << ")";
				//		return output.str();
				return Statement.GetName();
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSABinaryArithmeticStatement &stmt = (const SSABinaryArithmeticStatement &) Statement;

				if (stmt.Type == genc::BinaryOperator::RotateRight) {
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";

					int bits;
					switch (stmt.LHS()->GetType().Size()) {
						case 1:
							bits = 8;
							break;
						case 2:
							bits = 16;
							break;
						case 4:
							bits = 32;
							break;
						case 8:
							bits = 64;
							break;
						default:
							throw std::logic_error("Unsupported rotate-right data size");
					}

					output << "{"
					       "uint32_t lhs = " << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << ";"
					       "uint32_t rhs = " << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ";"
					       << stmt.GetName() << "= (lhs >> rhs) | (lhs << (" << bits << "-rhs));"
					       "}";

					return true;
				} else if (stmt.Type == genc::BinaryOperator::RotateRight) {
					throw std::logic_error("Unsupported rotate right");
				}

				// currently special case for SAR until new infrastructure for interpreter (and signed operations) developed
				if(stmt.Type == genc::BinaryOperator::SignedShiftRight) {
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << " /* signed */ >> " << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ");";
				} else {
					output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << Factory.GetOrCreate(stmt.LHS())->GetFixedValue() << genc::BinaryOperator::PrettyPrintOperator(stmt.Type) << Factory.GetOrCreate(stmt.RHS())->GetFixedValue() << ");";
				}

				return true;
			}
		};

		class SSACastStatementWalker : public SSAGenCWalker
		{
		public:

			SSACastStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			std::string GetFixedValue() const override
			{
				const SSACastStatement &stmt = (const SSACastStatement &) Statement;

				switch (stmt.GetCastType()) {
					case SSACastStatement::Cast_ZeroExtend:
					case SSACastStatement::Cast_SignExtend:
					case SSACastStatement::Cast_Truncate: {
						if (stmt.GetType().Reference) return Factory.GetOrCreate(stmt.Expr())->GetFixedValue();
						std::string cast_stmt = Factory.GetOrCreate(stmt.Expr())->GetFixedValue();
						
						// if we are switching signedness, change signedness  before extending
						if(stmt.Expr()->GetType().Signed != stmt.GetType().Signed) {
							SSAType partial_type = stmt.Expr()->GetType();
							partial_type.Signed = stmt.GetType().Signed;
							cast_stmt = "((" + partial_type.GetCType() + ")" + cast_stmt + ")";
						}
						cast_stmt = "((" + stmt.GetType().GetCType() + ")" + cast_stmt + ")";
						
						return cast_stmt;
					}
					case SSACastStatement::Cast_Reinterpret:
						switch (stmt.GetType().BaseType.PlainOldDataType) {
							case genc::IRPlainOldDataType::DOUBLE:
								return "(bitcast_u64_double(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
							case genc::IRPlainOldDataType::FLOAT:
								return "(bitcast_u32_float(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
							case genc::IRPlainOldDataType::INT64:
								return "(bitcast_double_u64(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
							case genc::IRPlainOldDataType::INT32:
								return "(bitcast_float_u32(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + "))";
						}

					case SSACastStatement::Cast_Convert:
						if(stmt.GetOption() != SSACastStatement::Option_RoundDefault) {
							UNIMPLEMENTED;
						}
						return "(" + stmt.GetType().GetCType() + ")(" + Factory.GetOrCreate(stmt.Expr())->GetFixedValue() + ")";

				}
				assert(false && "Unknown cast type");
				UNEXPECTED;
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const  override
			{
				return true;
				/*
				const SSACastStatement &stmt = (const SSACastStatement &)Statement;
				output << Statement.GetType().GetCType() << " " << Statement.GetName() << " = (" << stmt.TargetType.GetCType() << ")(" << Factory.GetOrCreate(stmt.Expr)->GetFixedValue() << ");";
				return true;*/
			}
		};

		class SSAConstantStatementWalker : public SSAGenCWalker
		{
		public:

			SSAConstantStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAConstantStatement &stmt = (const SSAConstantStatement &) Statement;
				return true;
			}

			virtual std::string GetFixedValue() const
			{
				const SSAConstantStatement &stmt = (const SSAConstantStatement &) Statement;
				std::stringstream str;
				if (stmt.GetType().IsFloating()) {
					switch (stmt.Constant.Type()) {
						case genc::IRConstant::Type_Float_Single:
							str << "(float)(" << stmt.Constant.Flt() << ")";
							break;
						case genc::IRConstant::Type_Float_Double:
							str << "(double)(" << stmt.Constant.Dbl() << ")";
							break;
						default:
							throw std::logic_error("");
					}

				} else {
					str << "(" << stmt.GetType().GetCType() << ")(" << stmt.Constant.Int() << "ULL)";
				}
				return str.str();
			}
		};

		class SSAFreeRegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSAFreeRegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			// Don't need to do anything here

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}
		};

		class SSAIfStatementWalker : public SSAGenCWalker
		{
		public:

			SSAIfStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAIfStatement &stmt = (const SSAIfStatement &) (Statement);

				output << "if(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") "
				       //						"  puts(\"C I T " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
				       "  goto __block_" << stmt.TrueTarget()->GetName() << ";"
				       "else "
				       //						"  puts(\"C I F " << stmt.GetName() << " " << stmt.GetISA() << " " << stmt.GetLineNumber() << "\");"
				       "  goto __block_" << stmt.FalseTarget()->GetName() << ";";
				return true;
			}
		};

		class SSADeviceReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSADeviceReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label, bool fully_fixed) const
			{
				const SSADeviceReadStatement &stmt = (const SSADeviceReadStatement&) (Statement);

				const SSAStatement *dev_id = stmt.Device();
				const SSAStatement *addr = stmt.Address();
				const SSASymbol *target = stmt.Target();

				output << "uint32_t " << stmt.GetName() << " = read_device(" << Factory.GetOrCreate(dev_id)->GetFixedValue() << ", " << Factory.GetOrCreate(addr)->GetFixedValue() << ", " << target->GetName() << ");";

				return true;
			}
		};

		class SSAIntrinsicStatementWalker : public SSAGenCWalker
		{
		public:

			SSAIntrinsicStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &) (Statement);

				switch (stmt.Type) {
					case SSAIntrinsicStatement::SSAIntrinsic_Clz32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_clz(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_TakeException:
						output << "take_exception(" << (Factory.GetOrCreate(stmt.Args(0))->GetFixedValue()) << "," << (Factory.GetOrCreate(stmt.Args(1))->GetFixedValue()) << ");";

						break;
					case SSAIntrinsicStatement::SSAIntrinsic_HaltCpu:
						output << "halt_cpu();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_Popcount32:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = __builtin_popcount(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
						// nothing here
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_Trap:
						output << "trap();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_SetCpuMode:
						output << "set_cpu_mode(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_WriteDevice:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = write_device(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_ProbeDevice:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = probe_device(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_PopInterrupt:
						output << "pop_interrupt();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_PushInterrupt:
						output << "push_interrupt(1);";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_GetCpuMode:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = get_cpu_mode();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_EnterKernelMode:
						output << "enter_kernel_mode();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_EnterUserMode:
						output << "enter_user_mode();";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_PendIRQ:
						output << "pend_interrupt();";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_DoubleAbs:
					case SSAIntrinsicStatement::SSAIntrinsic_FloatAbs:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = fabs(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsSnan:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = double_is_snan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_DoubleIsQnan:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = double_is_qnan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_FloatIsSnan:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = float_is_snan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_FloatIsQnan:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = float_is_qnan(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_DoubleSqrt:
					case SSAIntrinsicStatement::SSAIntrinsic_FloatSqrt:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = sqrt(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_AdcWithFlags:
						//SZ0A0P1C0000000V
						output << "{";
						output << "uint16_t flags = genc_adc_flags(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Args(2))->GetFixedValue() << ");";
						output << "write_register_C((flags >> 8) & 1);";
						output << "write_register_V(flags & 1);";
						output << "write_register_Z((flags >> 14) & 1);";
						output << "write_register_N((flags >> 15) & 1);";
						output << "}";
						break;

					/*
					case SSAIntrinsicStatement::SSAIntrinsic_RotateRight:
					output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
					output << "{"
						   "uint32_t lhs = " << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ";"
						   "uint32_t rhs = " << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ";"
						   << stmt.GetName() <<"= (lhs >> rhs) | (lhs << (32-rhs));"
						   "}";
					break;
					 */

					case SSAIntrinsicStatement::SSAIntrinsic_UpdateZN32:
						output << "write_register_Z(!(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "));";
						output << "write_register_N(((" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ") & 0x80000000) != 0);";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_UpdateZN64:
						output << "write_register_Z(!(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << "));";
						output << "write_register_N(((" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ") & 0x8000000000000000ull) != 0);";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_GetFeature:
						output << "uint32_t " << stmt.GetName() << " = GetFeatures().GetFeatureLevel(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ");";
						break;
					case SSAIntrinsicStatement::SSAIntrinsic_SetFeature:
						output << "GetFeatures().SetFeatureLevel(" << Factory.GetOrCreate(stmt.Args(0))->GetFixedValue() << ", " << Factory.GetOrCreate(stmt.Args(1))->GetFixedValue() << ");";
						break;

					case SSAIntrinsicStatement::SSAIntrinsic_FPGetFlush:
					case SSAIntrinsicStatement::SSAIntrinsic_FPGetRounding:
					case SSAIntrinsicStatement::SSAIntrinsic_FPSetFlush:
					case SSAIntrinsicStatement::SSAIntrinsic_FPSetRounding:
						if (stmt.HasValue()) {
							output << stmt.GetType().GetCType() << " " << stmt.GetName() << ";";
						}
						break;

					default:
						throw std::logic_error("Unrecognised intrinsic: " + std::to_string(stmt.Type));
				}
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAIntrinsicStatement &stmt = (const SSAIntrinsicStatement &) (Statement);
				switch (stmt.Type) {
					case SSAIntrinsicStatement::SSAIntrinsic_ReadPc:
						return "read_pc()";
					default:
						return stmt.GetName();
				}
			}
		};

		class SSAJumpStatementWalker : public SSAGenCWalker
		{
		public:

			SSAJumpStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAJumpStatement &stmt = (const SSAJumpStatement &) (Statement);

				output << "goto __block_" << stmt.Target()->GetName() << ";";
				return true;
			}
		};

		class SSAMemoryReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSAMemoryReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAMemoryReadStatement &stmt = (const SSAMemoryReadStatement &) (Statement);

				output << "{";
				output << "uint32_t _value = mem_read_" << (stmt.Width * 8) << (stmt.Signed ? "s" : "") << (stmt.User ? "_user" : "") << "(" << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << "," << stmt.Target()->GetName() << ");";
				output << "if(_value) {"
				       "take_exception(7, read_pc()+8);"
				       "longjmp(_longjmp_safepoint, 0);"
				       "}";
				output << "}";
				return true;
			}
		};

		class SSAMemoryWriteStatementWalker : public SSAGenCWalker
		{
		public:

			SSAMemoryWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAMemoryWriteStatement &stmt = (const SSAMemoryWriteStatement &) (Statement);
				output << "{";
				output << "uint32_t _value = mem_write_" << (stmt.Width * 8) << (stmt.User ? "_user" : "") << "(" << Factory.GetOrCreate(stmt.Addr())->GetFixedValue() << "," << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ");";
				output << "if(_value) {"
				       "if(_value <= 1024) {"
				       "	take_exception(7, read_pc()+8);"
				       "}"
				       "longjmp(_longjmp_safepoint, 0);"
				       "}";
				output << "}";
				return true;
			}
		};

		class SSAReadStructMemberStatementWalker : public SSAGenCWalker
		{
		public:

			SSAReadStructMemberStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAReadStructMemberStatement &stmt = (const SSAReadStructMemberStatement &) (Statement);
				std::stringstream str;

				if (stmt.MemberName == "IsPredicated")
					str << "inst.GetIsPredicated()";
				else {
					if (Statement.Parent->Parent->Isa->IsFieldOrthogonal(stmt.MemberName))
						str << "inst.GetField_" << Statement.Parent->Parent->Isa->ISAName << "_" << stmt.MemberName << "()";
					else
						str << "inst." << stmt.MemberName;
				}
				return str.str();
			}
		};

		class SSARegisterStatementWalker : public SSAGenCWalker
		{
		public:

			SSARegisterStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSARegisterStatement &stmt = (const SSARegisterStatement &) (Statement);

				auto &regfile = stmt.GetContext().GetArchDescription().GetRegFile();

				if (stmt.IsBanked) {
					auto &reg_desc = regfile.GetBankByIdx(stmt.Bank);
					auto idx_node = Factory.GetOrCreate(stmt.RegNum());

					if (stmt.IsRead) {
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = read_register_bank_" << reg_desc.ID << "(" << idx_node->GetFixedValue() << ");";
					} else {
						auto value_node = Factory.GetOrCreate(stmt.Value());
						output << "write_register_bank_" << reg_desc.ID << "(" << idx_node->GetFixedValue() << ", " << value_node->GetFixedValue() << ");";
					}
				} else {
					auto &reg_desc = regfile.GetSlotByIdx(stmt.Bank);

					if (stmt.IsRead) {
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = read_register_" << reg_desc.GetID() << "();";
					} else {
						auto value_node = Factory.GetOrCreate(stmt.Value());
						output << "write_register_" << reg_desc.GetID() << "(" << value_node->GetFixedValue() << ");";
					}
				}

				return true;
			}
		};

		class SSAReturnStatementWalker : public SSAGenCWalker
		{
		public:

			SSAReturnStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAReturnStatement &stmt = (const SSAReturnStatement&) this->Statement;
				if (stmt.Value()) {
					// write value to return register
					output << "_rval_ = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
				}
				output << "goto " << end_label << ";";
				return true;
			}
		};

		class SSASelectStatementWalker : public SSAGenCWalker
		{
		public:

			SSASelectStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSASelectStatement &stmt = (const SSASelectStatement &) (Statement);

				output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Cond())->GetFixedValue() << " ? (" << Factory.GetOrCreate(stmt.TrueVal())->GetFixedValue() << ") : (" << Factory.GetOrCreate(stmt.FalseVal())->GetFixedValue() << ");";

				return true;
			}
		};

		class SSASwitchStatementWalker : public SSAGenCWalker
		{
		public:

			SSASwitchStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSASwitchStatement &stmt = (const SSASwitchStatement &) (Statement);

				int i = 0;
				output << "switch(" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ") {";
				for (auto v : stmt.GetValues()) {
					output << "case (" << Factory.GetOrCreate(v.first)->GetFixedValue() << "): "
					       "  goto __block_" << v.second->GetName() << ";";
				}
				output << " default: "
				       "  goto __block_" << stmt.Default()->GetName() << "; "
				       "}";
				return true;
			}
		};

		class SSAVariableReadStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVariableReadStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAVariableReadStatement &stmt = (const SSAVariableReadStatement &) (Statement);
				return stmt.Target()->GetName();
			}
		};

		class SSAVariableWriteStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVariableWriteStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAVariableWriteStatement &stmt = (const SSAVariableWriteStatement &) (Statement);
				output << stmt.Target()->GetName() << " =  " << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
				return true;
			}
		};

		class SSACallStatementWalker : public SSAGenCWalker
		{
		public:

			SSACallStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSACallStatement &stmt = (const SSACallStatement&) (Statement);

				if (stmt.HasValue()) output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ";
				output << stmt.Target()->GetPrototype().GetIRSignature().GetName() << "(";

				bool first = true;
				for (int i = 0; i < stmt.ArgCount(); ++i) {
					if (!first) output << ",";
					first = false;

					const SSAStatement *arg_stmt = dynamic_cast<const SSAStatement*> (stmt.Arg(i));
					const SSASymbol *sym = dynamic_cast<const SSASymbol*> (stmt.Arg(i));
					if (arg_stmt != nullptr) {
						output << Factory.GetOrCreate(arg_stmt)->GetFixedValue();
					} else if (sym != nullptr) {
						output << sym->GetName();
					} else {
						throw std::logic_error("Unknown argument type");
					}
				}

				output << ");";
				return true;
			}
		};

		class SSAUnaryArithmeticStatementWalker : public SSAGenCWalker
		{
		public:

			SSAUnaryArithmeticStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAUnaryArithmeticStatement &stmt = (const SSAUnaryArithmeticStatement&) (Statement);
				switch (stmt.Type) {
						using namespace gensim::genc::SSAUnaryOperator;
					case OP_NEGATE:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = !" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
					case OP_COMPLEMENT:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = ~" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
					case OP_NEGATIVE:
						output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = -" << Factory.GetOrCreate(stmt.Expr())->GetFixedValue() << ";";
						break;
				}
				return true;
			}
		};

		class SSAVectorExtractElementStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVectorExtractElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				return true;
			}

			std::string GetFixedValue() const
			{
				const SSAVectorExtractElementStatement &stmt = (const SSAVectorExtractElementStatement&) (Statement);

				return Factory.GetOrCreate(stmt.Base())->GetFixedValue() + "[" + Factory.GetOrCreate(stmt.Index())->GetFixedValue() + "]";
			}
		};

		class SSAVectorInsertElementStatementWalker : public SSAGenCWalker
		{
		public:

			SSAVectorInsertElementStatementWalker(const SSAStatement &_statement, SSAWalkerFactory &_factory) : SSAGenCWalker(_statement, _factory)
			{
			}

			bool EmitFixedCode(util::cppformatstream &output, std::string end_label /* = 0 */, bool fully_fixed) const
			{
				const SSAVectorInsertElementStatement &stmt = (const SSAVectorInsertElementStatement&) (Statement);

				output << stmt.GetType().GetCType() << " " << stmt.GetName() << " = " << Factory.GetOrCreate(stmt.Base())->GetFixedValue() << ";";
				output << stmt.GetName() + "[" + Factory.GetOrCreate(stmt.Index())->GetFixedValue() + "] = " << Factory.GetOrCreate(stmt.Value())->GetFixedValue() << ";";
				return true;
			}

		};

	}

#define STMT_IS(x) std::type_index(typeid(genc::ssa::x)) == std::type_index(typeid(*stmt))
#define HANDLE(x) \
	if (STMT_IS(x)) return new GenCInterpreter::x##Walker(*stmt, *this)

	genc::ssa::SSANodeWalker *gensim::GenCInterpreterNodeFactory::Create(const genc::ssa::SSAStatement *stmt)
	{
		assert(stmt != NULL);

		HANDLE(SSABinaryArithmeticStatement);
		HANDLE(SSACastStatement);
		HANDLE(SSAConstantStatement);
		HANDLE(SSADeviceReadStatement);
		HANDLE(SSAIfStatement);
		HANDLE(SSAIntrinsicStatement);
		HANDLE(SSAJumpStatement);
		HANDLE(SSAMemoryReadStatement);
		HANDLE(SSAMemoryWriteStatement);
		HANDLE(SSAReadStructMemberStatement);
		HANDLE(SSARegisterStatement);
		HANDLE(SSAReturnStatement);
		HANDLE(SSASelectStatement);
		HANDLE(SSASwitchStatement);
		HANDLE(SSAVariableReadStatement);
		HANDLE(SSAVariableWriteStatement);
		HANDLE(SSAUnaryArithmeticStatement);
		HANDLE(SSAVectorExtractElementStatement);
		HANDLE(SSAVectorInsertElementStatement);
		HANDLE(SSACallStatement);
		assert(false && "Unrecognized statement type");
		return NULL;
	}
#undef STMT_IS
}
