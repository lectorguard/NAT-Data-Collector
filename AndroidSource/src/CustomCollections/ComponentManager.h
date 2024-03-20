#pragma once
#include <variant>
#include <array>
#include <memory>

template<typename ... Args>
struct ComponentManager
{
	using VariantType = typename std::variant<std::unique_ptr<Args> ...>;

	ComponentManager()
	{
		_components = { std::make_unique<Args>()... };
	}

	template<typename T>
	constexpr T& Get()
	{
		for (auto& variant : _components)
		{
			if (std::holds_alternative<std::unique_ptr<T>>(variant))
			{
				return *std::get<std::unique_ptr<T>>(variant);
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
					if (x)
					{
						cb(*x);
					}
					
				}, variant);
		};
	}

private:
	std::array<VariantType, std::variant_size_v<VariantType>> _components;
};