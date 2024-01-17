/*******************************************************************************
* @file     ThreadPool.h
* @brief    threadpool
* @author   linsn
* @date:    2021-9-16
******************************************************************************/
#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

typedef std::function<void()> callback;						/*处理函数*/
class ThreadPool
{
public:
	ThreadPool(int threadnum = 1, int maxwork = 10000);
	~ThreadPool();	
	
	bool append(callback func);									/*添加处理函数到队列*/
private:
	std::vector<std::thread> _pool;								/*线程池*/
	std::atomic<bool> _threadrun;								/*线程运行标志位，使用原子封装*/
	int _threadnum;												/*线程池中的线程数*/
	int _maxwork;												/*最大工作数*/
	std::mutex _mutex;											/*线程同步互斥量*/
	std::condition_variable _conv;								/*条件变量*/
	std::queue<callback> _workqueue;							/*处理函数队列*/
	void run();													/*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
};
