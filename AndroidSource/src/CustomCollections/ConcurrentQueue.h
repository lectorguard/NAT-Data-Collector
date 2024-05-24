// Created by Adam Kecskes
// https://github.com/K-Adam/SafeQueue

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <utility>
#include "SharedProtocol.h"
#include "CustomCollections/Log.h"

template<class T>
class ConcurrentQueue {

	std::queue<T> q;

	std::mutex mtx;
	std::condition_variable cv;

	std::condition_variable sync_wait;
	bool finish_processing = false;
	int sync_counter = 0;

	void DecreaseSyncCounter() {
		if (--sync_counter == 0) {
			sync_wait.notify_one();
		}
	}

public:

	typedef typename std::queue<T>::size_type size_type;

	ConcurrentQueue() {}

	~ConcurrentQueue() {
		Finish();
	}

	void Push(T&& item) {

		std::lock_guard<std::mutex> lock(mtx);

		q.push(std::move(item));
		cv.notify_one();

	}

	size_type Size() {

		std::lock_guard<std::mutex> lock(mtx);

		return q.size();

	}

	[[nodiscard]]
	bool TryPop(T& item) {

		std::lock_guard<std::mutex> lock(mtx);

		if (q.empty()) {
			return false;
		}

		item = std::move(q.front());
		q.pop();

		return true;

	}

	[[nodiscard]]
	bool Pop(T& item) {

		std::unique_lock<std::mutex> lock(mtx);

		sync_counter++;

		cv.wait(lock, [&] {
			return !q.empty() || finish_processing;
			});

		if (q.empty()) {
			DecreaseSyncCounter();
			return false;
		}

		item = std::move(q.front());
		q.pop();

		DecreaseSyncCounter();
		return true;

	}

	void Finish() {

		std::unique_lock<std::mutex> lock(mtx);

		finish_processing = true;
		cv.notify_all();

		sync_wait.wait(lock, [&]() {
			return sync_counter == 0;
			});

		finish_processing = false;

	}

	void push_error(shared::Error error)
	{
		using namespace shared;
		if (auto compressed_data = DataPackage::Create(error).Compress(false))
		{
			Push(std::move(*compressed_data));
		}
		else
		{
			Error compress_error{ ErrorType::ERROR, {"Failed to compress push error : " + (error.messages.empty() ? "" : error.messages[0]) } };
			if (auto compressed_data = DataPackage::Create(compress_error).Compress(false))
			{
				Push(std::move(*compressed_data));
			}
			else
			{
				Log::Error("Unable to compress error :  %s", error.messages.empty() ? "" : error.messages[0].c_str());
			}
		}
	}
};