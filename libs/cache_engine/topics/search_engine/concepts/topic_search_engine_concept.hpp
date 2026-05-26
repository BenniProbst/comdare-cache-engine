#pragma once
#include <concepts>
namespace comdare::cache_engine::search_engine::concepts {
struct SearchEngineTopicTag {};
template <typename T>
concept SearchEngineComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, SearchEngineTopicTag>;
}  // namespace
