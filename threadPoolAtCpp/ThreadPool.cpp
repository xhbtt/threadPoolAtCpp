#include "ThreadPool.h"
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>

using namespace std;

template <typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{
	do
	{
		taskQ = new TaskQueue<T>;
		if (taskQ == nullptr)
		{
			cout << "malloc taskQ fail...." << endl;
			break;
		}
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << "malloc threadIDs fail...." << endl;
			break;
		}
		memset(threadIDs, 0, sizeof(pthread_t) * max);
		minNum = min;
		maxNum = max;
		liveNum = min;
		exitNum = 0;

		if (pthread_mutex_init(&mutexpool, NULL) != 0 ||
			pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "mutex or condition fail" << endl;
			break;
		}

		shutdown = false;

		pthread_create(&managerID, NULL, manager, this);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
		}
		return;
	} while (0);

	if (threadIDs) delete[]threadIDs;
	if (taskQ) delete taskQ;

}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	//关闭线程池
	shutdown = true;
	//阻塞回收
	pthread_join(managerID, NULL);
	//唤醒阻塞的消费者
	for (int i = 0; i <		liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}
	//释放堆内存
	if (taskQ)
	{
		delete taskQ;
	}
	if (threadIDs)
	{
		delete[]threadIDs;
	}

	pthread_mutex_destroy(&mutexpool);
	pthread_cond_destroy(&notEmpty);
}

template <typename T>
void ThreadPool<T>::addTask(Task<T> task)
{
	if (shutdown)
	{
		return;
	}
	//添加任务
	taskQ->addTask(task);	
	pthread_cond_signal(&notEmpty);
}

template <typename T>
int ThreadPool<T>::getBusyNum()
{
	pthread_mutex_lock(&mutexpool);
	int busyNum = this->busyNum;
	pthread_mutex_unlock(&mutexpool);
	return busyNum;
}

template <typename T>
int ThreadPool<T>::getALiveNum()
{
	pthread_mutex_lock(&mutexpool);
	int liveNum = this->liveNum;
	pthread_mutex_unlock(&mutexpool);
	return liveNum;
}

template <typename T>
void* ThreadPool<T>::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (true)
	{
		pthread_mutex_lock(&pool->mutexpool);
		//当前任务队列是否为空
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//阻塞工作
			pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);

			//判断是否需要销毁线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexpool);
					pool->threadExit();
				}

			}
		}

		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexpool);
			pool->threadExit();
		}

		//从队列里取出一个任务
		Task<T> task = pool->taskQ->takeTask();

		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexpool);

		cout << "thread " << to_string(pthread_self()) << " start working...." << endl;
		
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		cout << "thread " << to_string(pthread_self()) << " end working...." << endl;
		
		pthread_mutex_lock(&pool->mutexpool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexpool);
	}
	return NULL;
}

template <typename T>
void* ThreadPool<T>::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (!pool->shutdown)
	{
		//每隔3s检测一次
		sleep(3);

		//取出任务数量和线程数量
		pthread_mutex_lock(&pool->mutexpool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexpool);

		//添加线程
		//任务个数>存活线程个数&&小于最大线程数
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			int counter = 0;
			for (int i = 0; i < pool->maxNum && counter < NUMBER
				&& pool->liveNum < pool->maxNum; ++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					counter++;
					pool->liveNum++;
				}

			}
			pthread_mutex_unlock(&pool->mutexpool);
		}

		//销毁线程
		//忙线程*2<存活的线程数 && 存活的线程数	> 最小的线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexpool);

			//让线程自杀
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}

	}
	return nullptr;
}

template <typename T>
void ThreadPool<T>::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] == tid)
		{
			threadIDs[i] = 0;
			cout << "threadExit() called, " << to_string(tid) << " exiting....\n";
			break;
		}
	}
	pthread_exit(NULL);
}
