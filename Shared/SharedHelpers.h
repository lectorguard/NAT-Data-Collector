#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include "JSerializer.h"

namespace shared::helper
{
		template<typename P, typename R>
		inline std::vector<R> mapVector(const std::vector<P>& vec, std::function<R(const P&)> lambda)
		{
			std::vector<R> buffer{ vec.size() };
			std::transform(vec.begin(), vec.end(), buffer.begin(), lambda);
			return buffer;
		}

		inline const std::vector<std::string> JserErrorToString(const std::vector<jser::JSerError>& errors)
		{
			return mapVector<jser::JSerError, std::string>(errors, [](auto jser_err) {return jser_err.Message; });
		}
}

