#pragma once
#include <memory>
#include <mutex>
#include <map>
#include <api/peer_connection_interface.h>
#include "json/json.h"
#include "app.h"

namespace vcprtc {
class RTCEngine : public IRTCEngine, public std::enable_shared_from_this<RTCEngine>
{
public:
    static std::shared_ptr<RTCEngine> instance()
    {
        static std::shared_ptr<RTCEngine> _instance;
        static std::once_flag ocf;
        std::call_once(ocf, []() {
            _instance.reset(new RTCEngine());
        });
        return _instance;
    }

    RTCEngine();
    ~RTCEngine();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> CreatePeerConnection(webrtc::PeerConnectionObserver* obs);
    int CreateVideoTrack(const Json::Value& config) override;
    int CreateAudioTrack(const Json::Value& config) override;
    int DestoryAudioTrack(const std::string& name) override;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> FetchVideoTrack(const std::string& name);
    rtc::scoped_refptr<webrtc::AudioTrackInterface> FetchAudioTrack(const std::string& name);
    rtc::Thread* getPeerEventHandler() { return _peerEventHandler.get(); }

    void addVideoSource(const std::string& name, webrtc::VideoTrackSourceInterface* source) override;
    void addVideoTrack(const std::string& name, webrtc::VideoTrackInterface* track) override;
    void addVideoSink(const std::string& name, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
    void removeVideoSource(const std::string& name) override;
    void removeVideoTrack(const std::string& name) override;
    void removeVideoSink(const std::string& name) override;
    void test0() override;
private:
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _pcFactory;
    std::unique_ptr<rtc::Thread> _signaling;
    std::unique_ptr<rtc::Thread> _peerEventHandler;

    std::map<std::string, rtc::scoped_refptr<webrtc::AudioTrackInterface>> _audioTracks;

    std::map<std::string, rtc::VideoSinkInterface<VideoFrame>*> _videoSinks;
    std::map<std::string, rtc::scoped_refptr<webrtc::VideoTrackInterface>> _videoTracks;
};

}