#pragma once
#include <rtc_base/thread.h>
#include "mqtt/async_client.h"
#include "mqtt/topic.h"
#include "ISignalClient.h"

namespace vcprtc {
class MqttSignalClient: public ISignalClient {
public:
	MqttSignalClient(const Json::Value& config, ISignalClientObsever* callback);
	~MqttSignalClient();
	virtual void sendText(const std::string& dst, const std::string& text) override;
	virtual void sendJson(const Json::Value& val) override;
	//virtual void handleMessage(const std::string& text);
	//virtual void onSignalClosed();
	
private:
	int subcribeTopics(const Json::Value& topics);
	int connect(const Json::Value& config);
	int disconnect();
	
private:
	mqtt::async_client_ptr _cli;
	int _pubQos;
	bool _pubRetained;
};
}
