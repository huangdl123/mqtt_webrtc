#pragma once
#include <string>
#include <memory>
#include <stdint.h>
#include "json/json.h"
#include "logger/logger.h"
#include <rtc_base/third_party/sigslot/sigslot.h>
#include <api/video/video_frame.h>
#include <api/media_stream_interface.h>
#include <api/video/video_sink_interface.h>


namespace vcprtc {
typedef struct
{
	int width;
	int height;
	int format;
	const uint8_t* pixes[4];
	int stride[4];
} stVideoFrame;

class IRTCEngine
{
public:
	static std::shared_ptr<IRTCEngine> instance();
	virtual int CreateVideoTrack(const Json::Value& config) = 0;
	virtual int CreateAudioTrack(const Json::Value& config) = 0;
	virtual int DestoryAudioTrack(const std::string& name) = 0;

	virtual void addVideoSource(const std::string& name, webrtc::VideoTrackSourceInterface* source) = 0;
	virtual void addVideoTrack(const std::string& name, webrtc::VideoTrackInterface* track) = 0;
	virtual void addVideoSink(const std::string& name, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) = 0;
	virtual void removeVideoSource(const std::string& name) = 0;
	virtual void removeVideoTrack(const std::string& name) = 0;
	virtual void removeVideoSink(const std::string& name) = 0;

	virtual void test0() = 0;
};

class IPeerClient
{
public:
	virtual const std::string& getSid() = 0;
	virtual void muteVideo(bool mute) = 0;
	virtual void setLocalVideo(const std::string& video) = 0;
	virtual void sendMessage(const std::string& msg) = 0;
	virtual void sendData(void* data, int size) = 0;
public:
	sigslot::signal3<const std::string&, void*, int> onData;
};

class IApp
{
public:
	static std::shared_ptr<IApp> create(const std::string& clientId);
	virtual std::string clientId() = 0;
	virtual int call(const std::string& sid) = 0;
	virtual int answer(const std::string& sid) = 0;
	virtual int hangup(const std::string& sid) = 0;
	virtual IPeerClient* getPeer(const std::string& sid) = 0;
public:
	sigslot::signal1<const std::string&> onNewPeerClient;
	sigslot::signal1<const std::string&> onDeletePeerClient;
};

typedef webrtc::VideoFrame VideoFrame;
typedef rtc::VideoSinkInterface<webrtc::VideoFrame>  IVideoRenderer;

}
