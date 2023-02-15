#pragma once

#include <mutex>

#include <ge/utils/UniqueName.hpp>

#define LOCK(x) std::lock_guard<std::mutex> PRIVATE_NAME(lock)(x);
