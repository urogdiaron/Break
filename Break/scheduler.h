#pragma once
#include "../../Ecs/Project1/view.h"
#include "ftl/atomic_counter.h"
#include "ftl/task_scheduler.h"
#include <functional>

namespace ecs
{
	struct Scheduler
	{
		Scheduler()
			: mainCounter(&taskScheduler)
			, countersRunning
			{ 
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
				ftl::AtomicCounter(&taskScheduler),
			}
		{
			taskScheduler.Init({ 400, 0, ftl::EmptyQueueBehavior::Sleep });
		}

		template<class Fn, class... Ts>
		static ftl::TaskFunction createTaskFunction()
		{
			auto fn = [](ftl::TaskScheduler* taskScheduler, void* arg) -> void
			{
				auto tuple = reinterpret_cast<std::tuple<View<Ts...>*, int, Fn, int>*>(arg);
				auto view = std::get<0>(*tuple);
				auto iChunk = std::get<1>(*tuple);
				auto job = std::get<2>(*tuple);
				auto counterIndex = std::get<3>(*tuple);

				char blockName[64];
				sprintf_s(blockName, "Task MT (%d)", counterIndex);
				EASY_NONSCOPED_BLOCK(blockName);

				for (auto it = view->beginForChunk(iChunk); it != view->endForChunk(); ++it)
				{
					std::apply(job, *it);
				}

				EASY_END_BLOCK;
			};

			return fn;
		}

		void setupSystemGroup(Ecs* ecs, void (*setupFunction)(Ecs*, Scheduler*, int))
		{
			auto fnWrapperTask = [](ftl::TaskScheduler* taskScheduler, void* args)
			{
				auto& [ecs, scheduler, setupFunction, counterIndex] = *reinterpret_cast<std::tuple<Ecs*, Scheduler*, void (*)(Ecs*, Scheduler*, int), int>*>(args);
				setupFunction(ecs, scheduler, counterIndex);
			};

			int systemGroupIndex = createCounter();
			auto argTuple = std::make_tuple(ecs, this, setupFunction, systemGroupIndex);
			int bufferIndex = currentBufferIndex.fetch_add(sizeof(argTuple));
			uint8_t* buffer = &argBuffer[bufferIndex];
			memcpy(buffer, &argTuple, sizeof(argTuple));

			ftl::Task task;
			task.ArgData = buffer;
			task.Function = fnWrapperTask;
			taskScheduler.AddTasks(1, &task, &mainCounter);
		}

		void setupTasks(Ecs* ecs, int systemGroupIndex, void (*setupFunction)(Ecs*, Scheduler*, int))
		{
			taskScheduler.WaitForCounter(&countersRunning[systemGroupIndex], 0, false);
			auto fnWrapperTask = [](ftl::TaskScheduler* taskScheduler, void* args)
			{
				auto& [ecs, scheduler, setupFunction, counterIndex] = *reinterpret_cast<std::tuple<Ecs*, Scheduler*, void (*)(Ecs*, Scheduler*, int), int>*>(args);
				setupFunction(ecs, scheduler, counterIndex);
			};

			int taskCounterIndex = createCounter();
			auto argTuple = std::make_tuple(ecs, this, setupFunction, taskCounterIndex);
			int bufferIndex = currentBufferIndex.fetch_add(sizeof(argTuple));
			uint8_t* buffer = &argBuffer[bufferIndex];
			memcpy(buffer, &argTuple, sizeof(argTuple));

			ftl::Task task;
			task.ArgData = buffer;
			task.Function = fnWrapperTask;
			taskScheduler.AddTasks(1, &task, &countersRunning[systemGroupIndex]);
		}

		template <class Fn, class... Ts>
		void addTask(int counterIndex, View<Ts...>* view, Fn&& job)		// Called from a fiber
		{
			taskScheduler.WaitForCounter(&countersRunning[counterIndex], 0, false);
			view->initializeData();
			int chunkCount = (int)view->queriedChunks_.size();
			if (!chunkCount)
				return;

			{
				EASY_BLOCK("Adding chunk tasks");
				std::vector<ftl::Task> tasks(chunkCount);
				for (int i = 0; i < chunkCount; i++)
				{
					auto argTuple = std::make_tuple(view, i, job, counterIndex);
					int bufferIndex = currentBufferIndex.fetch_add(sizeof(argTuple));
					uint8_t* buffer = &argBuffer[bufferIndex];
					memcpy(buffer, &argTuple, sizeof(argTuple));

					tasks[i].ArgData = buffer;
					tasks[i].Function = Scheduler::createTaskFunction<Fn, Ts...>();
				}
				taskScheduler.AddTasks(chunkCount, tasks.data(), &countersRunning[counterIndex]);
			}
			taskScheduler.WaitForCounter(&countersRunning[counterIndex], 0, false);
		}

		template <class Fn, class... Ts>
		void add(int counterIndex, View<Ts...>* view, Fn&& job)
		{
			//EASY_FUNCTION();
			if (singleThreadedMode)
			{
				view->initializeData();
				int chunkCount = (int)view->queriedChunks_.size();
				for (int i = 0; i < chunkCount; i++)
				{
					EASY_BLOCK("Task (ST)");
					for (auto it = view->beginForChunk(i); it != view->endForChunk(); ++it)
					{
						std::apply(job, *it);
					}
				}
				return;
			}

			auto fnWrapperTask = [](ftl::TaskScheduler* taskScheduler, void* arg) -> void
			{
				//EASY_BLOCK("Wrapper Task");
				auto argTuple = reinterpret_cast<std::tuple<Scheduler*, View<Ts...>*, Fn, int>*>(arg);
				auto& scheduler = std::get<0>(*argTuple);
				auto& view = std::get<1>(*argTuple);
				auto& job = std::get<2>(*argTuple);
				auto& counterIndex = std::get<3>(*argTuple);

				taskScheduler->WaitForCounter(&scheduler->countersRunning[counterIndex], 0, false);

				view->initializeData();
				int chunkCount = (int)view->queriedChunks_.size();
				if (!chunkCount)
					return;

				EASY_BLOCK("Adding chunk tasks");
				std::vector<ftl::Task> tasks(chunkCount);
				for (int i = 0; i < chunkCount; i++)
				{
					auto argTuple = std::make_tuple(view, i, job, counterIndex);
					int bufferIndex = scheduler->currentBufferIndex.fetch_add(sizeof(argTuple));
					uint8_t* buffer = &scheduler->argBuffer[bufferIndex];
					memcpy(buffer, &argTuple, sizeof(argTuple));

					tasks[i].ArgData = buffer;
					tasks[i].Function = Scheduler::createTaskFunction<Fn, Ts...>();
				}
				taskScheduler->AddTasks(chunkCount, tasks.data(), &scheduler->countersRunning[counterIndex]);
			};

			auto argTuple = std::make_tuple(this, view, job, counterIndex);
			int bufferIndex = currentBufferIndex.fetch_add(sizeof(argTuple));
			uint8_t* buffer = &argBuffer[bufferIndex];
			memcpy(buffer, &argTuple, sizeof(argTuple));

			ftl::Task task;
			task.ArgData = buffer;
			task.Function = fnWrapperTask;
			taskScheduler.WaitForCounter(&mainCounter, 0, true);
			taskScheduler.AddTasks(1, &task, &mainCounter);
		}

		int createCounter()
		{
			int index = currentCounterIndex.fetch_add(1);
			_ASSERT_EXPR(index < countersRunning.size(), L"Ran out of counters, most likely because c++ is too bad!");
			return index;
		}

		void waitAll(Ecs* ecs)
		{
			int counterCount = currentCounterIndex;
			taskScheduler.WaitForCounter(&mainCounter, 0, true);
			for (int i = 0; i < counterCount; i++)
			{
				taskScheduler.WaitForCounter(&countersRunning[i], 0, true);
			}

			currentCounterIndex = 0;
			currentBufferIndex = 0;

			ecs->executeCommmandBuffer();
		}

		ftl::TaskScheduler taskScheduler;

		std::mutex argBufferMutex;
		std::array<uint8_t, (1 << 20)> argBuffer; // 1 MB
		std::atomic<int> currentBufferIndex = 0;

		ftl::AtomicCounter mainCounter;
		std::array<ftl::AtomicCounter, 8> countersRunning;
		std::atomic<int> currentCounterIndex = 0;
		bool singleThreadedMode = false;
	};
}
