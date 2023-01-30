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
	//�ر��̳߳�
	shutdown = true;
	//��������
	pthread_join(managerID, NULL);
	//����������������
	for (int i = 0; i <		liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}
	//�ͷŶ��ڴ�
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
	//�������
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
		//��ǰ��������Ƿ�Ϊ��
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//��������
			pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);

			//�ж��Ƿ���Ҫ�����߳�
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

		//�Ӷ�����ȡ��һ������
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
		//ÿ��3s���һ��
		sleep(3);

		//ȡ�������������߳�����
		pthread_mutex_lock(&pool->mutexpool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexpool);

		//����߳�
		//�������>����̸߳���&&С������߳���
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

		//�����߳�
		//æ�߳�*2<�����߳��� && �����߳���	> ��С���߳���
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexpool);

			//���߳���ɱ
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
