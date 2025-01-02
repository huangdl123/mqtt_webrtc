/**
 * This file is part of janus_client project.
 * Author:    Jackie Ou
 * Created:   2020-10-01
 **/

#pragma once

#include <memory>

#if !defined(SPDLOG_ACTIVE_LEVEL)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include "spdlog/spdlog.h"

namespace vcprtc {

class RTCLogSink;

class Logger
{
public:
	Logger();

	~Logger();

	static void init();

	static void destroy();

	static std::shared_ptr<spdlog::logger>& rtcLogger();

	static std::shared_ptr<spdlog::logger>& appLogger();

private:
	static std::shared_ptr<spdlog::logger> _appLogger;

	static std::shared_ptr<spdlog::logger> _rtcLogger;

	static std::unique_ptr<RTCLogSink> _rtcLogSink;
};

}

#define TLOG_RTC(...) SPDLOG_LOGGER_TRACE(vcprtc::Logger::rtcLogger(), __VA_ARGS__)
#define DLOG_RTC(...) SPDLOG_LOGGER_DEBUG(vcprtc::Logger::rtcLogger(), __VA_ARGS__)
#define ILOG_RTC(...) SPDLOG_LOGGER_INFO(vcprtc::Logger::rtcLogger(), __VA_ARGS__)
#define WLOG_RTC(...) SPDLOG_LOGGER_WARN(vcprtc::Logger::rtcLogger(), __VA_ARGS__)
#define ELOG_RTC(...) SPDLOG_LOGGER_ERROR(vcprtc::Logger::rtcLogger(), __VA_ARGS__)
#define CLOG_RTC(...) SPDLOG_LOGGER_CRITICAL(vcprtc::Logger::rtcLogger(), __VA_ARGS__)

#define TLOG(...) SPDLOG_LOGGER_TRACE(vcprtc::Logger::appLogger(), __VA_ARGS__)
#define DLOG(...) SPDLOG_LOGGER_DEBUG(vcprtc::Logger::appLogger(), __VA_ARGS__)
#define ILOG(...) SPDLOG_LOGGER_INFO(vcprtc::Logger::appLogger(), __VA_ARGS__)
#define WLOG(...) SPDLOG_LOGGER_WARN(vcprtc::Logger::appLogger(), __VA_ARGS__)
#define ELOG(...) SPDLOG_LOGGER_ERROR(vcprtc::Logger::appLogger(), __VA_ARGS__)
#define CLOG(...) SPDLOG_LOGGER_CRITICAL(vcprtc::Logger::appLogger(), __VA_ARGS__)
