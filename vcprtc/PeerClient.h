#pragma once

#include <api/peer_connection_interface.h>
#include "json/json.h"
#include "app.h"


class IPeerClientObsever
{
public:
	virtual void sendWebrtcSignal(const std::string& sid, const Json::Value& payload) = 0;
	virtual void onPeerClosed(const std::string& sid) = 0;
};

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "amstrmid.lib")
#pragma comment(lib, "msdmo.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define PEER_ROLE_PUBLISHER    0
#define PEER_ROLE_SUBSCRIBER   1

namespace vcprtc {
class PeerClient : public webrtc::PeerConnectionObserver, 
	               public webrtc::CreateSessionDescriptionObserver,
	               public webrtc::DataChannelObserver,
	               public IPeerClient,
				   public std::enable_shared_from_this<PeerClient>
{
public:
	static rtc::scoped_refptr<PeerClient> Create(IPeerClientObsever* sc, const Json::Value& config);
	PeerClient(IPeerClientObsever* sc, const Json::Value& config);
	~PeerClient();
	void start();
	//void stop(); //not needed
	
	void handleSignal(const Json::Value& msg);
	int getRole() { return _role; }
	const std::string& getSid() override { return _sid; }
	void muteVideo(bool mute) override;
	void setLocalVideo(const std::string& video);
	void sendMessage(const std::string& msg) override;
	void sendData(void* data, int size) override;

	void muteVideo2(bool mute);

	// webrtc::PeerConnectionObserver
	void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
	void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver, const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
	void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;// {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
	void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;
	void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}
	
	// webrtc::CreateSessionDescriptionObserver
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;

	//DataChannelObserver
	void OnStateChange() override {};
	void OnMessage(const webrtc::DataBuffer& buffer) override;
protected:
	webrtc::MediaStreamTrackInterface* getStreamTrack(int type); //0: local audio; 1: local video; 2: remote audio; 3: remote video
	void addStreams();
	void addDataChannel();
	void sendMessage(int msgId, const Json::Value& msg);
	void handleMessage(int msgId, const Json::Value& msg);

	std::unique_ptr<webrtc::SessionDescriptionInterface> convertToSdp(const Json::Value& msg);
	std::unique_ptr<webrtc::IceCandidateInterface> convertToCandidate(const Json::Value& msg);
	rtc::Thread* eventHandler();
private:
	bool _loopback;
	int _role;  //caller callee
	std::string _sid;
	std::string _localVideoName;
	std::string _localAudioName;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
	rtc::scoped_refptr<webrtc::DataChannelInterface> _dataChannel;

	webrtc::AudioTrackSinkInterface* _localAudioSink;
	webrtc::AudioTrackSinkInterface* _remoteAudioSink;

	IPeerClientObsever* _sc;
};

}