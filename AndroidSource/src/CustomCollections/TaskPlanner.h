#pragma once

#include <iostream>
#include <functional>
#include <tuple>
#include <type_traits>
#include <optional>
#include <variant>

#define MEMFUN(x) std::bind(x, this, std::placeholders::_1)


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
	std::variant<Exec<R>, R> tasks;
	bool IsReady = false;

	void Update()
	{
		if (Exec<R>* ec = std::get_if<Exec<R>>(&tasks))
		{
			tasks = std::move(ec->func().tasks);
		}
		else
		{
			IsReady = true;
		}
	}


	std::optional<R> GetResult()
	{
		if (auto* res = std::get_if<R>(&tasks))
		{
			return std::move(*res);
		}
		else
		{
			return std::nullopt;
		}
	}
};


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
	TaskExecutor<R> Evaluate(PAR&& param)
	{
		return Evaluate_exec<0, R, PAR, FUNCS...>(std::move(param), objects);
	}


private:
	template<size_t I = 0, typename R, typename PAR, typename...Cs>
	static TaskExecutor<R> Evaluate_exec(PAR&& par, std::tuple<Cs...> tup)
	{
		auto& func = std::get<I>(tup);


		// res must be optional
		if (auto res{ func(std::forward<PAR>(par)) })
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
			return { Exec<R>{ [val = std::forward<PAR>(par), tup]() { return Evaluate_exec<I, R>(std::move(val), tup); } } };
		}
	}

	std::tuple<FUNCS...> objects;
};


// int main()
// {
// 	std::cout << "Hello World!\n";
// 
// 	bool shouldReturn = false;
// 
// 	std::function<std::optional<int>(std::string)> func1 = [](auto URL) {
// 		std::cout << "Send HTTP to " << URL << std::endl;
// 		return 5;
// 		};
// 
// 	auto func2 = [&shouldReturn](int httpHandle) -> std::optional<float>
// 		{
// 			std::cout << "Received Request " << httpHandle << std::endl;
// 			if (shouldReturn)
// 			{
// 				return std::optional(5.5f);
// 			}
// 			else
// 			{
// 				return std::nullopt;
// 			}
// 		};
// 
// 	std::function<std::optional<std::string>(float)> func3 = [](float in)
// 		{
// 			std::cout << "Yess goes on" << in << std::endl;
// 			return "yeet";
// 		};
// 
// 	using namespace std;
// 
// 	TaskPlanner t1 =
// 		TaskPlanner(func1)
// 		.AndThen<optional<float>, int>(func2)
// 		.AndThen(func3);
// 
// 
// 	TaskExecutor<std::string> runner = t1.Evaluate<std::string>("url");
// 
// 	for (int i = 0; i < 10; ++i)
// 	{
// 		runner.Update();
// 		if (i == 6)
// 		{
// 			shouldReturn = true;
// 		}
// 	}
// 	if (auto finalres = runner.GetResult())
// 	{
// 		std::cout << *finalres << std::endl;
// 	}
// }


