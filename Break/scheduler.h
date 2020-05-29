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
		{
			taskScheduler.Init({ 400, 4, ftl::EmptyQueueBehavior::Sleep });
		}

		template<class Fn, class... Ts>
		ftl::TaskFunction createTaskFunction()
		{
			auto fn = [](ftl::TaskScheduler* taskScheduler, void* arg) -> void
			{
				EASY_BLOCK("Task (MT)");
				auto tuple = reinterpret_cast<std::tuple<View<Ts...>*, int, Fn>*>(arg);
				auto view = std::get<0>(*tuple);
				auto iChunk = std::get<1>(*tuple);
				auto job = std::get<2>(*tuple);
				for (auto it = view->beginForChunk(iChunk); it != view->endForChunk(); ++it)
				{
					std::apply(job, *it);
				}
			};

			return fn;
		}

		template <class Fn, class... Ts>
		void add(View<Ts...>&& view, Fn&& job)
		{
			EASY_FUNCTION();
			if (singleThreadedMode)
			{
				view.initializeData();
				int chunkCount = (int)view.queriedChunks_.size();
				for (int i = 0; i < chunkCount; i++)
				{
					EASY_BLOCK("Task (ST)");
					for (auto it = view.beginForChunk(i); it != view.endForChunk(); ++it)
					{
						std::apply(job, *it);
					}
				}
				return;
			}

			view.initializeData();
			int chunkCount = (int)view.queriedChunks_.size();
			if (!chunkCount)
				return;

			EASY_BLOCK("Task Setup");
			ftl::AtomicCounter counter(&taskScheduler);

			std::vector<decltype(std::make_tuple(&view, chunkCount, job))> args;
			std::vector<ftl::Task> tasks(chunkCount);

			args.reserve(chunkCount);

			for (int i = 0; i < chunkCount; i++)
			{
				auto& arg = args.emplace_back(std::make_tuple(&view, i, job));
				tasks[i].ArgData = &arg;
				tasks[i].Function = createTaskFunction<Fn, Ts...>();
			}
			EASY_END_BLOCK;

			{
				EASY_BLOCK("AddTasks");
				taskScheduler.AddTasks(chunkCount, tasks.data(), &counter);
			}
			{
				EASY_BLOCK("WaitForCounter");
				taskScheduler.WaitForCounter(&counter, 0, true);
			}
		}

		void waitAll()
		{
			EASY_FUNCTION();
		}

		ftl::TaskScheduler taskScheduler;
		bool singleThreadedMode = false;
	};
}
