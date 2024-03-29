﻿#include "ThreadPool.h"

ThreadPool::ThreadPool(int threadnum, int maxwork)
	: _threadnum(threadnum), _maxwork(maxwork), _threadrun(true)
{
	if (_threadnum > 0 && _maxwork > 0)
	{
		for (int i = 0; i < _threadnum; ++i)
		{
			_pool.emplace_back([this]
							   { run(); });
		}
	}
}

ThreadPool::~ThreadPool()
{
	_threadrun = false;
	_conv.notify_all();

	/* 等待任务结束， 前提：线程一定会执行完 */
	for (std::thread &thread : _pool)
	{
		if (thread.joinable())
			thread.join();
	}

	if (!_workqueue.empty())
	{
		std::queue<callback> empty;
		swap(empty, _workqueue);
	}
	_pool.clear();
}

bool ThreadPool::append(callback func)
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (static_cast<int>(_workqueue.size()) >= _maxwork)
	{
		return false;
	}

	_workqueue.push(func);

	_conv.notify_one();

	return true;
}

void ThreadPool::run()
{
	while (_threadrun)
	{
		do
		{
			/*出作用域后解锁（只在判断是否为假唤醒时加锁，后面执行不加锁）*/
			std::unique_lock<std::mutex> lock(_mutex);
#if 0
			/*循环判断，防止假唤醒*/
			while (_workqueue.empty() && _threadrun)
			{
				_conv.wait(lock);
			}
#else
			/*wait函数有第二个参数也能防止假唤醒*/
			_conv.wait(lock, [this]
					   {
				if (_workqueue.empty() && _threadrun)
					return false;
				return true; });
#endif
		} while (-1 == __LINE__);

		if (!_threadrun)
			break;

		if (!_workqueue.empty())
		{
			callback func = _workqueue.front();
			_workqueue.pop();

			func();
		}
	}
}