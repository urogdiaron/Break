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
				auto tuple = reinterpret_cast<std::tuple<View<Ts...>*, int, Fn, int, const char*>*>(arg);
				auto view = std::get<0>(*tuple);
				auto iChunk = std::get<1>(*tuple);
				auto job = std::get<2>(*tuple);
				auto counterIndex = std::get<3>(*tuple);
				auto name = std::get<4>(*tuple);

				char blockName[64];
				sprintf_s(blockName, "Task MT %s (%d)", name, counterIndex);
				EASY_NONSCOPED_BLOCK(blockName);

				for (auto it = view->beginForChunk(iChunk); it != view->endForChunk(); ++it)
				{
					std::apply(job, *it);
				}

				EASY_END_BLOCK;
			};

			return fn;
		}

		template<class TSystem>
		void scheduleSystem(Ecs* ecs, int systemGroupIndex);
		void runSystems();

		template <class Fn, class... Ts>
		void addTask(int counterIndex, View<Ts...>* view, Fn&& job, const char* name)		// Called from a fiber
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
					auto argTuple = std::make_tuple(view, i, job, counterIndex, name);
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

			systems.clear();
			ecs->executeCommmandBuffer();
		}

		ftl::TaskScheduler taskScheduler;

		std::mutex argBufferMutex;
		std::array<uint8_t, (1 << 20)> argBuffer; // 1 MB
		std::atomic<int> currentBufferIndex = 0;

		ftl::AtomicCounter mainCounter;
		std::array<ftl::AtomicCounter, 8> countersRunning;
		std::atomic<int> currentCounterIndex = 0;

		std::vector<std::unique_ptr<struct System>> systems;
		bool singleThreadedMode = false;
	};

	template<class... Ts>
	struct Job
	{
		template<class Fn>
		Job(View<Ts...>&& view, Fn&& jobFunction)
			: view(std::move(view))
			, fn(jobFunction)
		{
		}

		std::function<void(const View<Ts...>::iterator<true>&, const entityId&, Ts&...)> fn;
		View<Ts...> view;
	};

	struct System
	{
		virtual void scheduleJobs() = 0;

		template<class... Ts>
		void ScheduleJob(Job<Ts...>& job, const char* name = "")
		{
			scheduler->addTask(systemIndex, &job.view, std::move(job.fn), name);
		}

		Ecs* ecs;
		Scheduler* scheduler;
		int systemGroupIndex;
		int systemIndex;
	};

	template<class TSystem>
	void Scheduler::scheduleSystem(Ecs* ecs, int systemGroupIndex)
	{
		auto& systemPtr = systems.emplace_back(std::make_unique<TSystem>());
		System* system = systemPtr.get();

		system->ecs = ecs;
		system->scheduler = this;
		system->systemGroupIndex = systemGroupIndex;
		system->systemIndex = createCounter();
	}

	void Scheduler::runSystems()
	{
		auto fnWrapperTask = [](ftl::TaskScheduler* taskScheduler, void* args)
		{
			auto& [scheduler, systemGroupIndex] = *reinterpret_cast<std::tuple<Scheduler*, int>*>(args);
			for (auto& system : scheduler->systems)
			{
				if (system->systemGroupIndex == systemGroupIndex)
				{
					system->scheduleJobs();
					taskScheduler->WaitForCounter(&scheduler->countersRunning[system->systemIndex], 0, false);
				}
			}
		};

		std::vector<int> uniqueGroupIndices;
		for (auto& system : systems)
		{
			auto it = std::find(uniqueGroupIndices.begin(), uniqueGroupIndices.end(), system->systemGroupIndex);
			if (it == uniqueGroupIndices.end())
				uniqueGroupIndices.push_back(system->systemGroupIndex);
		}

		for (int groupIndex : uniqueGroupIndices)
		{
			auto argTuple = std::make_tuple(this, groupIndex);
			int bufferIndex = currentBufferIndex.fetch_add(sizeof(argTuple));
			uint8_t* buffer = &argBuffer[bufferIndex];
			memcpy(buffer, &argTuple, sizeof(argTuple));

			ftl::Task task;
			task.ArgData = buffer;
			task.Function = fnWrapperTask;
			taskScheduler.AddTasks(1, &task, &countersRunning[groupIndex]);
		}
	}
}
