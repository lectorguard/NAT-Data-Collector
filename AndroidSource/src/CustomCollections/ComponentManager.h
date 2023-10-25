#pragma once
#include <variant>
#include <array>

template<typename ... Args>
struct ComponentManager
{
	using VariantType = typename std::variant<Args ...>;

	ComponentManager()
	{
		using TupleType = typename std::tuple<Args...>;
		std::apply([&](auto&& ... args) { _components = { {std::move(args) ...} }; }, TupleType());
	}

	template<typename T>
	constexpr T& Get()
	{
		for (auto& variant : _components)
		{
			if (std::holds_alternative<T>(variant))
			{
				return std::get<T>(variant);
			};
		};
		throw std::runtime_error("Type must exist");
	}

	template<typename T>
	void ForEach(const T& cb)
	{
		for (auto& variant : _components)
		{
			std::visit([cb](auto&& x)
				{
					cb(x);
				}, variant);
		};
	}

private:
	std::array<VariantType, std::variant_size_v<VariantType>> _components;
};