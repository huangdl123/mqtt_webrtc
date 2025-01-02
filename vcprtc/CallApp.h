#pragma once
#include <memory>
#include <map>
#include "json/json.h"
#include "PeerClient.h"
#include "AudioRender.h"
#include "SignalClient/ISignalClient.h"
#include "app.h"

namespace vcprtc {
class CallApp :
	public IApp,
	public ISignalClientObsever,
	public IPeerClientObsever
{
public:
	static std::shared_ptr<CallApp> Create(const std::string& name);
	CallApp(const std::string& name);
	~CallApp();

	int call(const std::string& sid) override;
	int answer(const std::string& sid) override;
	int hangup(const std::string& sid) override;
	IPeerClient* getPeer(const std::string& sid) override;
	

	std::string clientId() override { return _clientId; }

	void sendWebrtcSignal(const std::string& sid, const Json::Value& payload) override;
	void onPeerClosed(const std::string& sid) override;
	void onSignalMessage(const std::string& text) override;
	void onSignalClosed() override;
	rtc::Thread* eventThread() override;
protected:
	std::string getSendRouter(const std::string& sid);
	void send(const std::string& dst, const Json::Value& payload);
	void handleMessage(const std::string& text);

	rtc::scoped_refptr<PeerClient> getPeerClient(const std::string& sid);
	void startNewPeer(const Json::Value& config);
	void stopPeerClients(int type);
	void stopPeerClients(const std::string& sid);
	void stopAllPeerClients();
private:
	bool _looping;
	std::string _clientId;
	std::unique_ptr<rtc::Thread> _eventHandlerThread;
	std::shared_ptr<ISignalClient> _signal;

	std::map<std::string, rtc::scoped_refptr<PeerClient>> _peers;

	std::shared_ptr<vcprtc::AudioFrameObserver> _nullAudioObs;
	rtc::scoped_refptr<vcprtc::AudioRender> _localAudioRender;
	rtc::scoped_refptr<vcprtc::AudioRender> _remoteAudioRender;
};
}