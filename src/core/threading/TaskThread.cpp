/* Copyright (c) 2024 No Tomorrow Games. All rights reserved. */

#include <core/threading/TaskThread.hpp>

#include <core/profiling/ProfileScope.hpp>

namespace hyperion {
namespace threading {

TaskThread::TaskThread(const ThreadID &thread_id, ThreadPriorityValue priority)
    : Thread(thread_id, priority),
      m_is_running(false),
      m_stop_requested(false),
      m_num_tasks(0)
{
}

TaskThread::TaskThread(Name name, ThreadPriorityValue priority)
    : Thread(name, priority),
      m_is_running(false),
      m_stop_requested(false),
      m_num_tasks(0)
{
}

void TaskThread::Stop()
{
    m_stop_requested.Set(true, MemoryOrder::RELAXED);

    m_scheduler.RequestStop();
    
    m_is_running.Set(false, MemoryOrder::RELAXED);
}

void TaskThread::operator()()
{
    m_is_running.Set(true, MemoryOrder::RELAXED);
    m_num_tasks.Set(m_task_queue.Size(), MemoryOrder::RELEASE);

    while (!m_stop_requested.Get(MemoryOrder::RELAXED)) {
        if (m_task_queue.Empty()) {
            if (!m_scheduler.WaitForTasks(m_task_queue)) {
                Stop();

                break;
            }
        } else {
            // append all tasks from the scheduler
            m_scheduler.AcceptAll(m_task_queue);
        }

        HYP_PROFILE_BEGIN;

        const uint32 num_tasks = m_task_queue.Size();
        m_num_tasks.Set(num_tasks, MemoryOrder::RELEASE);

        BeforeExecuteTasks();

        {
            HYP_NAMED_SCOPE("Executing tasks");
        
            // execute all tasks outside of lock
            while (m_task_queue.Any()) {
                m_task_queue.Pop().Execute();
            }
            
            m_num_tasks.Decrement(num_tasks, MemoryOrder::RELEASE);
        }

        AfterExecuteTasks();
    }

    m_is_running.Set(false, MemoryOrder::RELAXED);
}
} // namespace threading
} // namespace hyperion