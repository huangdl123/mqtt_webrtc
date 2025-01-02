#include "RtcEngine.h"
#include <rtc_base/ssl_adapter.h>
#include <api/create_peerconnection_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include "MyCapturer.h"


namespace vcprtc {
RTCEngine::RTCEngine()
{
	rtc::InitializeSSL();
	_peerEventHandler = rtc::Thread::Create();
	_peerEventHandler->SetName("peer event handler", nullptr);
	_peerEventHandler->Start();

	_signaling = rtc::Thread::Create();
	_signaling->SetName("pc_signaling_thread", nullptr);
	_signaling->Start();
	_pcFactory = webrtc::CreatePeerConnectionFactory(
		nullptr /* network_thread */, nullptr /* worker_thread */,
		_signaling.get() /* signaling_thread */, nullptr /* default_adm */,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
		nullptr /* audio_processing */);
}

RTCEngine::~RTCEngine()
{
	_videoTracks.clear();
	_audioTracks.clear();
	_peerEventHandler = nullptr;
	_pcFactory = nullptr;
	_signaling = nullptr;
	rtc::CleanupSSL();
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface> RTCEngine::CreatePeerConnection(webrtc::PeerConnectionObserver* obs)
{
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = "stun:stun.l.google.com:19302";

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.enable_dtls_srtp = true;
	config.servers.push_back(server);

	auto pc = _pcFactory->CreatePeerConnection(config, nullptr, nullptr, obs);
	return pc;
}

int RTCEngine::CreateVideoTrack(const Json::Value& config)
{
	std::string name = config["name"].asString();
	if ("" == name) return -1;
	auto it = _videoTracks.find(name);
	if (it != _videoTracks.end())
	{
		return 0;
	}
	
	rtc::scoped_refptr<MyCapturer> videoSrc = new rtc::RefCountedObject<MyCapturer>();
	videoSrc->startCapturer();
	addVideoSource(name, videoSrc);
	return 0;
}

int RTCEngine::CreateAudioTrack(const Json::Value& config)
{
	return 0;
}

int RTCEngine::DestoryAudioTrack(const std::string& name)
{
	auto it = _audioTracks.find(name);
	if (it != _audioTracks.end())
	{
		_audioTracks.erase(it);
	}
	return 0;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface> RTCEngine::FetchVideoTrack(const std::string& name)
{
	if ("" == name) return nullptr;
	auto it = _videoTracks.find(name);
	return it == _videoTracks.end() ? nullptr : it->second;
}

rtc::scoped_refptr<webrtc::AudioTrackInterface> RTCEngine::FetchAudioTrack(const std::string& name)
{
	auto it = _audioTracks.find(name);
	return it == _audioTracks.end() ? nullptr : it->second;
}

void RTCEngine::addVideoSource(const std::string& name, webrtc::VideoTrackSourceInterface* source)
{
	//lock
	if (NULL == source) return;
	auto oldTrack = _videoTracks.find(name);
	if (oldTrack != _videoTracks.end())
	{
		_videoTracks.erase(oldTrack);
	}
	rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrk(_pcFactory->CreateVideoTrack(name, source));
	_videoTracks[name] = videoTrk;

	auto sink = _videoSinks.find(name);
	if (sink != _videoSinks.end() && NULL != sink->second)
	{
		videoTrk->AddOrUpdateSink(sink->second, rtc::VideoSinkWants());
	}
}

void RTCEngine::addVideoTrack(const std::string& name, webrtc::VideoTrackInterface* track)
{
	//lock
	if (NULL == track) return;
	_videoTracks[name] = track;

	auto sink = _videoSinks.find(name);
	if (sink != _videoSinks.end() && NULL != sink->second)
	{
		track->AddOrUpdateSink(sink->second, rtc::VideoSinkWants());
	}
}

void RTCEngine::addVideoSink(const std::string& name, rtc::VideoSinkInterface<VideoFrame>* sink)
{
	//lock
	if (NULL == sink) return;
	auto track = _videoTracks.find(name);
	_videoSinks[name] = sink;
	if (track != _videoTracks.end())
	{
		if(track->second) track->second->AddOrUpdateSink(sink, rtc::VideoSinkWants());
	}
}

void RTCEngine::removeVideoSource(const std::string& name)
{
	auto it = _videoTracks.find(name);
	if (it != _videoTracks.end())
	{
		_videoTracks.erase(it);
	}
}

void RTCEngine::removeVideoTrack(const std::string& name)
{
	auto it = _videoTracks.find(name);
	if (it != _videoTracks.end())
	{
		_videoTracks.erase(it);
	}
}

void RTCEngine::removeVideoSink(const std::string& name)
{
	auto it = _videoSinks.find(name);
	if (it != _videoSinks.end())
	{
		auto track = _videoTracks.find(name);
		if (track != _videoTracks.end())
		{
			if(track->second) track->second->RemoveSink(it->second);
		}
		_videoSinks.erase(it);
	}
}

void RTCEngine::test0()
{
}

}