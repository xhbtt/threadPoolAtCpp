#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"
#include <pthread.h>

template <typename T>
class ThreadPool
{
public:
	//�����̳߳�
	ThreadPool(int min, int max);

	//�����̳߳�
	~ThreadPool();

	//���̳߳��������
	void addTask(Task<T> task);

	//��ǰ�̳߳����ж��ٸ��߳��ڹ���
	int getBusyNum();

	//��ȡ�̳߳��л��ŵ��߳�
	int getALiveNum();

private:
	static void* worker(void* arg);
	static void* manager(void* arg);
	void threadExit();

private:
	TaskQueue<T>* taskQ;

	pthread_t managerID;	//�������߳�ID
	pthread_t* threadIDs;	//�������߳�ID
	int minNum;	//��С�߳���
	int maxNum;	//����߳���
	int busyNum;	//æ���߳���
	int liveNum;	//�����̸߳���
	int exitNum;	//Ҫ���ٵ��߳���
	pthread_mutex_t mutexpool; //���̳߳�����
	pthread_cond_t notEmpty;	//�ж���������Ƿ��ǿյ�
	static const int NUMBER = 2;

	bool shutdown;	//�Ƿ������̳߳�����Ϊtrue������Ϊfalse
};
