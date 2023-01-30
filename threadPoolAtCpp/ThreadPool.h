#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"
#include <pthread.h>

template <typename T>
class ThreadPool
{
public:
	//创建线程池
	ThreadPool(int min, int max);

	//销毁线程池
	~ThreadPool();

	//给线程池添加任务
	void addTask(Task<T> task);

	//当前线程池中有多少个线程在工作
	int getBusyNum();

	//获取线程池中活着的线程
	int getALiveNum();

private:
	static void* worker(void* arg);
	static void* manager(void* arg);
	void threadExit();

private:
	TaskQueue<T>* taskQ;

	pthread_t managerID;	//管理者线程ID
	pthread_t* threadIDs;	//工作者线程ID
	int minNum;	//最小线程数
	int maxNum;	//最大线程数
	int busyNum;	//忙的线程数
	int liveNum;	//存活的线程个数
	int exitNum;	//要销毁的线程数
	pthread_mutex_t mutexpool; //给线程池上锁
	pthread_cond_t notEmpty;	//判断任务队列是否是空的
	static const int NUMBER = 2;

	bool shutdown;	//是否销毁线程池销毁为true不销毁为false
};
