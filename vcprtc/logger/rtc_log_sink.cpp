/**
 * This file is part of janus_client project.
 * Author:    Jackie Ou
 * Created:   2020-10-16
 **/

#include "rtc_log_sink.h"
#include "logger.h"

namespace vcprtc
{
	void RTCLogSink::OnLogMessage(const std::string& msg, rtc::LoggingSeverity severity, const char* tag) 
	{
		
	}

	void RTCLogSink::OnLogMessage(const std::string& message, rtc::LoggingSeverity severity) 
	{
		TLOG_RTC(message);
	}

	void RTCLogSink::OnLogMessage(const std::string& message) 
	{
		
	}
}