/*
 * genC/ssa/statement/SSAConstantStatement.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ir/IRConstant.h"
#include "genC/ssa/statement/SSAStatement.h"

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			/**
			 * A statement representing a single constant value.
			 */
			class SSAConstantStatement : public SSAStatement
			{
			public:
				IRConstant Constant;

				virtual bool IsFixed() const;
				bool HasSideEffects() const override;
				bool Resolve(DiagnosticContext& ctx) override;

				virtual void PrettyPrint(std::ostringstream &) const;
				virtual std::set<SSASymbol *> GetKilledVariables();

				void Accept(SSAStatementVisitor& visitor) override;

				SSAConstantStatement(SSABlock *parent, const IRConstant &value, const IRType &type, SSAStatement *before = NULL);

				const SSAType GetType() const override
				{
					return _constant_type;
				}

			private:
				const SSAType _constant_type;
				SSAConstantStatement();
			};
		}
	}
}