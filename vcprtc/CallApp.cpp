#include "CallApp.h"
#include "logger/logger.h"

#define WEBRTC_PEER_PREFIX    "webrtc_"
#define CLIENT_SID_PREFIX     "webrtc/client/"

namespace vcprtc {
std::shared_ptr<CallApp> CallApp::Create(const std::string& name)
{
	std::shared_ptr<CallApp> instance = std::make_shared<CallApp>(name);
	return instance;
}

CallApp::CallApp(const std::string& name)
{
	_localAudioRender = vcprtc::AudioRender::Create("local audio", _nullAudioObs);
	_remoteAudioRender = vcprtc::AudioRender::Create("remote audio", _nullAudioObs);
	
	_eventHandlerThread = rtc::Thread::Create();
	_eventHandlerThread->SetName("signal client", nullptr);
	_eventHandlerThread->Start();
	_looping = true;

	_clientId = name;

	Json::Value config;
	config["server"] = "mqtt://47.97.102.134:11893";
	config["clientId"] = _clientId;
	config["username"] = "huanglilei";
	config["password"] = "123456";

	Json::Value subTopic;
	subTopic["topic"] = getSendRouter(config["clientId"].asString());
	subTopic["qos"] = 2;
	subTopic["retained"] = 0;
	subTopic["noLocal"] = 0;
	config["subscribes"].append(subTopic);

	Json::Value pubConfig;
	pubConfig["qos"] = 2;
	pubConfig["retained"] = 0;
	config["publish"] = pubConfig;

	Json::Value will;
	will["topic"] = "webrtc/action/leave";
	will["qos"] = 2;
	will["msg"] = "go";
	will["retained"] = 0;
	config["will"] = will;

	_signal = ISignalClient::create(MQTT_SIGNAL_TYPE, config, this);
}

CallApp::~CallApp()
{
	stopAllPeerClients();
	_looping = false;
}

void CallApp::send(const std::string& dst, const Json::Value& msg)
{
	Json::FastWriter writer;
	std::string text = writer.write(msg);
	if (!_looping) return;
	_eventHandlerThread->PostTask(RTC_FROM_HERE, [signal=_signal, dst, text]() {
		signal->sendText(dst, text);
	});
}

void CallApp::onSignalMessage(const std::string& text)
{
	if (!_looping) return;
	_eventHandlerThread->PostTask(RTC_FROM_HERE, [self = this, text]() {
		ILOG(text);
		self->handleMessage(text);
	});
}

void CallApp::onSignalClosed()
{
	ILOG("Signal closed");
	stopAllPeerClients();
}

rtc::Thread* CallApp::eventThread()
{
	return _looping ? _eventHandlerThread.get() : NULL;
}

rtc::scoped_refptr<PeerClient> CallApp::getPeerClient(const std::string& sid)
{
	auto it = _peers.find(sid);
	return it == _peers.end() ? nullptr : it->second;
}

void CallApp::startNewPeer(const Json::Value& config)
{
	auto pc = PeerClient::Create(this, config);
	_peers[pc->getSid()] = pc;
	onNewPeerClient.emit(pc->getSid());
	pc->start();
}

void CallApp::stopPeerClients(int type)
{
	auto it = _peers.begin();
	while (it != _peers.end()) {
		if (type == it->second->getRole()) {
			onDeletePeerClient.emit(it->second->getSid());
			it = _peers.erase(it);
		}
		else {
			++it;
		}
	}
}

void CallApp::stopPeerClients(const std::string& sid)
{
	auto it = _peers.begin();
	while (it != _peers.end()) {
		if (sid == it->second->getSid()) {
			onDeletePeerClient.emit(it->second->getSid());
			it = _peers.erase(it);
		}
		else {
			++it;
		}
	}
}

void CallApp::stopAllPeerClients()
{
	while (_peers.size() > 0)
	{
		auto it = _peers.begin();
		hangup(it->first);
	}
}

std::string CallApp::getSendRouter(const std::string& sid)
{
	return CLIENT_SID_PREFIX + sid;
}

void CallApp::sendWebrtcSignal(const std::string& sid, const Json::Value& payload)
{
	Json::Value msg;
	msg["type"] = WEBRTC_PEER_PREFIX + payload["type"].asString();
	msg["sid"] = _clientId;
	msg["payload"] = payload;
	send(getSendRouter(sid), msg);
}

void CallApp::onPeerClosed(const std::string& sid)
{
	if (!_looping) return;
	_eventHandlerThread->PostTask(RTC_FROM_HERE, [self = this, sid]() {
		self->hangup(sid);
	});
}

void CallApp::handleMessage(const std::string& text)
{
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(text, root))
	{
		WLOG("WebsocketClient json  parse error: {}", text);
		return;
	}
	if (!root.isMember("type") || !root.isMember("sid") || !root.isMember("payload"))
	{
		WLOG("WebsocketClient json  has no selected member: {}",text);
		return;
	}
	std::string type = root["type"].asString();
	std::string sid = root["sid"].asString();
	Json::Value payload = root["payload"];
	if (type == "call" || type == "answer")
	{
		int role = ("answer" == type) ? PEER_ROLE_PUBLISHER : PEER_ROLE_SUBSCRIBER;
		Json::Value config;
		config["role"] = role;
		config["sid"] = sid;
		config["localVideo"] = "video0";

		stopPeerClients(sid);
		startNewPeer(config);
		if (type == "call")
		{
			answer(sid);
		}
	}
	else if (type == "hangup")
	{
		stopPeerClients(sid);
	}
	else if (type.find(WEBRTC_PEER_PREFIX) == 0)
	{
		auto pc = getPeerClient(sid);
		if(pc) pc->handleSignal(payload);
	}
}

int CallApp::call(const std::string& sid)
{
	Json::Value msg;
	Json::Value payload;
	msg["type"] = "call";
	msg["sid"] = _clientId;
	msg["payload"] = payload;
	send(getSendRouter(sid), msg);
	return 0;
}

int CallApp::answer(const std::string& sid)
{
	Json::Value msg;
	Json::Value payload;
	msg["type"] = "answer";
	msg["sid"] = _clientId;
	msg["payload"] = payload;
	send(getSendRouter(sid), msg);
	return 0;
}

int CallApp::hangup(const std::string& sid)
{
	Json::Value msg;
	Json::Value payload;
	msg["type"] = "hangup";
	msg["sid"] = _clientId;
	msg["payload"] = payload;
	send(getSendRouter(sid), msg);
	stopPeerClients(sid);
	return 0;
}

IPeerClient* CallApp::getPeer(const std::string& sid)
{
	auto it = _peers.find(sid);
	if (it == _peers.end())
	{
		return NULL;
	}
	return it->second == nullptr ? NULL : it->second.get();
}

}