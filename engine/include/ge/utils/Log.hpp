#pragma once

#define SPDLOG_COMPILED_LIB
#include <spdlog/spdlog.h>

#define GE_SET_LOG_LEVEL spdlog::set_level

#define GE_LOG_LEVEL_INFO spdlog::level::info
#define GE_LOG_LEVEL_DEBUG spdlog::level::debug
#define GE_LOG_LEVEL_TRACE spdlog::level::trace
#define GE_LOG_LEVEL_WARN spdlog::level::warn
#define GE_LOG_LEVEL_ERROR spdlog::level::error
#define GE_LOG_LEVEL_CRITICAL spdlog::level::critical

#define GE_INFO spdlog::info
#define GE_DEBUG spdlog::debug
#define GE_TRACE spdlog::trace
#define GE_WARN spdlog::warn
#define GE_ERROR spdlog::error
#define GE_CRITICAL spdlog::critical