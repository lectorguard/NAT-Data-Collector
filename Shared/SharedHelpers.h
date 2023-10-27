#pragma once
#include <vector>
#include <functional>
#include <algorithm>

namespace shared_data::helper
{
		template<typename P, typename R>
		inline std::vector<R> mapVector(const std::vector<P>& vec, std::function<R(const P&)> lambda)
		{
			std::vector<R> buffer{ vec.size() };
			std::transform(vec.begin(), vec.end(), buffer.begin(), lambda);
			return buffer;
		}
}

