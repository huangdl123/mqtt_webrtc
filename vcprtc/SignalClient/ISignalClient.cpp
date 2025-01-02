#include "ISignalClient.h"
#include "MqttSignalClient.h"

namespace vcprtc {
std::shared_ptr<ISignalClient> ISignalClient::create(int type, const Json::Value& config, ISignalClientObsever* callback)
{
	std::shared_ptr<ISignalClient> client = nullptr;
	switch (type)
	{
	case MQTT_SIGNAL_TYPE:
		client = std::make_shared<MqttSignalClient>(config, callback);
		break;
	default:
		;
	}
	return client;
}

ISignalClient::ISignalClient(ISignalClientObsever* callback)
{
	_callback = callback;
	if (_callback && _callback->eventThread()) return;

	_internalThread = rtc::Thread::Create();
	_internalThread->SetName("signal_client", nullptr);
	_internalThread->Start();
}

rtc::Thread* ISignalClient::eventThread()
{
	if (_callback && _callback->eventThread())
	{
		return _callback->eventThread();
	}
	return _internalThread.get();
}

void ISignalClient::onSignalMessage(const std::string& text)
{
	if (NULL == _callback) return;
	_callback->onSignalMessage(text);
}

void ISignalClient::onSignalClosed()
{
	if (NULL == _callback) return;
	_callback->onSignalClosed();
}

}