#pragma once

#include <iostream>
#include <functional>
#include <tuple>
#include <type_traits>
#include <optional>
#include <variant>
#include "SharedProtocol.h"

#define MEMFUN(x) std::bind(x, this, std::placeholders::_1, std::placeholders::_2)


struct TaskStatus
{
	shared::ServerResponse value;
};

enum class TaskExecType
{
	RUNNING,
	HAS_RESULT,
	ERROR
};

template<typename R>
struct TaskExecutor;

template<typename R>
struct Exec
{
	std::function<TaskExecutor<R>()> func = nullptr;
};

template<typename R>
struct TaskExecutor
{
	std::variant<Exec<R>, R, TaskStatus> tasks = TaskStatus{ shared::ServerResponse::OK() };
	TaskExecType Status = TaskExecType::RUNNING;

	void Update()
	{
		if (Exec<R>* ec = std::get_if<Exec<R>>(&tasks))
		{
			tasks = ec->func().tasks;
			Status = TaskExecType::RUNNING;
		}
		else if (TaskStatus* status = std::get_if<TaskStatus>(&tasks))
		{
			Status = TaskExecType::ERROR;
		}
		else
		{
			Status = TaskExecType::HAS_RESULT;
		}
	}

	bool IsReady() { return Status != TaskExecType::RUNNING; }

	std::optional<R> GetResult()
	{
		if (auto* res = std::get_if<R>(&tasks))
		{
			return *res;
		}
		else
		{
			return std::nullopt;
		}
	}

	std::optional<shared::ServerResponse> GetError()
	{
		if (auto* res = std::get_if<TaskStatus>(&tasks))
		{
			return res->value;
		}
		else
		{
			return std::nullopt;
		}
	}
};


// Each passed function must return a value as optional
// If nullopt, the function will be called again until an actual value is returned
// Every next function gets the previous return value as passed parameter
// Every function gets a second ServerResponse parameter as ref
// If an error is set as response, the whole chain will instantly fail with this error
template<typename...FUNCS>
class TaskPlanner
{

public:
	TaskPlanner(std::tuple<FUNCS...> tup) :objects(tup) {}
	TaskPlanner(FUNCS... lambdas) :objects(std::forward_as_tuple(lambdas...)) {}

	template<typename T>
	TaskPlanner<FUNCS..., T> AndThen(T rhs) const {

		return std::tuple_cat(objects, std::make_tuple(rhs));
	}

	template<typename R, typename PAR>
	TaskExecutor<R> Evaluate(PAR param)
	{
		return Evaluate_exec<0, R, PAR, FUNCS...>(std::move(param), objects);
	}


private:
	template<size_t I = 0, typename R, typename PAR, typename...Cs>
	static TaskExecutor<R> Evaluate_exec(PAR par, std::tuple<Cs...> tup)
	{
		TaskStatus stat{};
		auto& func = std::get<I>(tup);
		// res must be optional
		// Object must be copyable, pass pointer if object not movable
		auto res{ func(par, stat.value) };
		if (!stat.value)
		{
			return TaskExecutor<R>{stat};
		}

		if (res)
		{
			if constexpr (I + 1 != sizeof...(Cs))
			{
				return Evaluate_exec<I + 1, R>(std::move(*res), tup);
			}
			else
			{
				using RESTYPE = typename std::remove_reference_t<decltype(*res)>;

				static_assert(std::is_same_v<R, RESTYPE>, "Result type must match");
				return { std::move(*res) };
			}
		}
		else
		{
			return { Exec<R>{ [pm = std::move(par), tup]() mutable { return Evaluate_exec<I, R>(std::move(pm), tup); } }};
		}
	}

	std::tuple<FUNCS...> objects;
};

