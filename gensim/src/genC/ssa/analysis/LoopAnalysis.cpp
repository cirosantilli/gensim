/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "define.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAFormAction.h"

using namespace gensim::genc::ssa;

bool LoopExists(SSABlock *block, const std::unordered_map<SSABlock*, int> &block_idxs, std::vector<bool> &visited, std::vector<bool> &rec_stack)
{
	int block_idx = block_idxs.at(block);
	if(visited[block_idx] == false) {

		visited[block_idx] = true;
		rec_stack[block_idx] = true;

		for(auto succ : block->GetSuccessors()) {
			int succ_idx = block_idxs.at(succ);

			if(!visited[succ_idx] && LoopExists(succ, block_idxs, visited, rec_stack)) {
				return true;
			} else if(rec_stack[succ_idx]) {
				return true;
			}
		}
	}

	rec_stack[block_idx] = false;
	return false;
}

bool LoopExists(const SSAFormAction& action)
{
	std::unordered_map<SSABlock*, int> block_idxs;
	// perform a depth first traversal of the graph
	std::vector<bool> visited (action.Blocks.size(), false);
	std::vector<bool> rec_stack (action.Blocks.size(), false);

	for(auto i : action.Blocks) {
		block_idxs[i] = block_idxs.size();
	}

	return LoopExists(action.EntryBlock, block_idxs, visited, rec_stack);
}

LoopAnalysisResult LoopAnalysis::Analyse(const SSAFormAction& action)
{
	LoopAnalysisResult result;
	result.LoopExists = LoopExists(action);
	return result;
}