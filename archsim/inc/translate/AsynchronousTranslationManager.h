/*
 * File:   AsynchronousTranslationManager.h
 * Author: s0457958
 *
 * Created on 06 August 2014, 09:16
 */

#ifndef ASYNCHRONOUSTRANSLATIONMANAGER_H
#define	ASYNCHRONOUSTRANSLATIONMANAGER_H

#include "translate/TranslationManager.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <unordered_set>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace translate
	{
		namespace profile
		{
			class Region;
		}

		class AsynchronousTranslationWorker;

		class WorkUnitQueueComparator
		{
		public:
			bool operator()(const TranslationWorkUnit* lhs, const TranslationWorkUnit* rhs) const;
		};

		class AsynchronousTranslationManager : public TranslationManager
		{
			friend class AsynchronousTranslationWorker;

		public:
			AsynchronousTranslationManager(util::PubSubContext *psctx);
			~AsynchronousTranslationManager();

			bool Initialise() override;
			void Destroy() override;

			void UpdateThreshold() override;

			bool TranslateRegion(gensim::Processor& cpu, profile::Region& rgn, uint32_t weight);

			void PrintStatistics(std::ostream& stream);

		private:
			/**
			 * List maintaining asynchronous worker threads.
			 */
			std::list<AsynchronousTranslationWorker *> workers;

			/**
			 * Lock and condition protecting the work unit queue.
			 */
			std::mutex work_unit_queue_lock;
			std::condition_variable work_unit_queue_cond;

			//typedef std::priority_queue<TranslationWorkUnit *, std::vector<TranslationWorkUnit *>, WorkUnitQueueComparator> work_unit_queue_t;
			typedef std::priority_queue<TranslationWorkUnit *, std::vector<TranslationWorkUnit*>, WorkUnitQueueComparator> work_unit_queue_t;

			/**
			 * Queue of translation work units awaiting translation.
			 * Manager pushes to BACK of queue
			 * Workers pop from FRONT of queue
			 */
			work_unit_queue_t work_unit_queue;
		};
	}
}

#endif	/* ASYNCHRONOUSTRANSLATIONMANAGER_H */
