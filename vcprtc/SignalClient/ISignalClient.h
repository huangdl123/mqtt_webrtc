#pragma once
#include <rtc_base/thread.h>
#include "json/json.h"

#define MQTT_SIGNAL_TYPE        0
#define WEBSOCKET_SIGNAL_TYPE   1

namespace vcprtc {
class ISignalClientObsever {
public:
	virtual void onSignalMessage(const std::string& text) = 0;
	virtual void onSignalClosed() = 0;
	virtual rtc::Thread* eventThread() = 0;
};

class ISignalClient {
public:
	static std::shared_ptr<ISignalClient> create(int type, const Json::Value& config, ISignalClientObsever* callback);
	virtual void sendText(const std::string& dst, const std::string& text) = 0;
	virtual void sendJson(const Json::Value& val) = 0;
protected:
	ISignalClient(ISignalClientObsever* callback);
	rtc::Thread* eventThread();
	void onSignalMessage(const std::string& text);
	void onSignalClosed();
private:
	ISignalClientObsever* _callback;
	std::unique_ptr<rtc::Thread> _internalThread;
};
}