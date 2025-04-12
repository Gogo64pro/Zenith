#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace zenith {

struct StringHash {
	using is_transparent = void;
	[[nodiscard]] size_t operator()(const char *txt) const {
		return std::hash<std::string_view>{}(txt);
	}
	[[nodiscard]] size_t operator()(std::string_view txt) const {
		return std::hash<std::string_view>{}(txt);
	}
	[[nodiscard]] size_t operator()(const std::string &txt) const {
		return std::hash<std::string>{}(txt);
	}
};

template <typename V>
using StringHashMap = std::unordered_map<std::string, V, StringHash, std::equal_to<>>;

} // zenith