#include <gtest/gtest.h>

#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"

using namespace gensim::genc::testing;
using namespace gensim::genc::ssa;

TEST(GenC_Parse, TestExternalCall)
{
    // source code to test
    const std::string sourcecode = R"||(
helper void testfn()
{
	__builtin_external_call();
	return;
}
    )||";
    // parse code
	
	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);
	
	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
    auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);   
    
	ASSERT_NE(nullptr, ctx);
	
    // actually perform test
    ASSERT_EQ(true, ctx->HasAction("testfn"));
	ASSERT_EQ(false, ctx->HasAction("notatestfn"));
}