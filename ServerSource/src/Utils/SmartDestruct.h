#pragma once
#include <functional>

namespace SD
{
	template<typename T>
	struct SmartDestruct
	{
		SmartDestruct() {};

		SmartDestruct(T* value, const std::function<void(T*)>& shutdown) : Value(value), _shutdown(shutdown)
		{
		}

		~SmartDestruct()
		{
			_shutdown(Value);
		}

		T* Value;
	private:
		std::function<void(T*)> _shutdown = nullptr;
	};

	// If type factory is not implemented, compilation will fail
	template<typename T>
	struct SmartDestructFactory {};

	template<typename T, typename ...Args>
	SmartDestruct<T> Create(Args&&... args)
	{
		// If this call fails, there is no factory type for T. Please add the factory type above. 
		return SmartDestructFactory<T>::Create(std::forward<Args>(args)...);
	}
}
