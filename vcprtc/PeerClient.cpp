#include "PeerClient.h"
//#include <api/create_peerconnection_factory.h>
//#include <api/audio_codecs/builtin_audio_decoder_factory.h>
//#include <api/audio_codecs/builtin_audio_encoder_factory.h>
//#include <api/video_codecs/builtin_video_encoder_factory.h>
//#include <api/video_codecs/builtin_video_decoder_factory.h>
//#include "MyCapturer.h"
#include "logger/logger.h"
#include "RtcEngine.h"

#define MSG_SIGNAL_SEND   0

namespace vcprtc {

class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver 
{
public:
    static DummySetSessionDescriptionObserver* Create() 
    {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() { ILOG("Sdp success"); }
    virtual void OnFailure(webrtc::RTCError error)
    {
        std::string errType = std::string(webrtc::ToString(error.type()));
        WLOG("Sdp failed: {}, {}", errType, error.message());
    }
};

rtc::scoped_refptr<PeerClient> PeerClient::Create(IPeerClientObsever* sc, const Json::Value& config)
{
    rtc::scoped_refptr<PeerClient> client = new rtc::RefCountedObject<PeerClient>(sc, config);
    return client;
}

PeerClient::PeerClient(IPeerClientObsever* sc, const Json::Value& config):
    _sc(sc)
{
    _loopback = false;

    _role = config["role"].asInt();
    _sid = config["sid"].asString();
    _localVideoName = config["localVideo"].asString();
    _localAudioName = config["localAudio"].asString();
}

PeerClient::~PeerClient()
{
    _pc = nullptr;
    ILOG("Peer({}, {}) release...", _role, _sid);
}

void PeerClient::start()
{
    _pc = RTCEngine::instance()->CreatePeerConnection(this);
    addStreams();
    
    if (PEER_ROLE_PUBLISHER == _role)
    {
        addDataChannel();
        //createOffer();
        _pc->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
}

void PeerClient::addDataChannel()
{
    _dataChannel = _pc->CreateDataChannel("myDataChannel", nullptr);
    //std::unique_ptr<webrtc::DataChannelObserver> data_channel_observer(new webrtc::DataChannelObserver());
    //rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
    //    peer_connection->CreateDataChannel("myDataChannel", nullptr);
    //data_channel_observer->SetDataChannel(data_channel.get());
    _dataChannel->RegisterObserver(this);
}

void PeerClient::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    _dataChannel = channel;
    _dataChannel->RegisterObserver(this);
    ILOG("Data channel created by remote peer.");
}

void PeerClient::sendMessage(const std::string& msg)
{
    if (!_dataChannel) return;
    _dataChannel->Send(webrtc::DataBuffer(msg));
}

void PeerClient::sendData(void* data, int size)
{
    if (!_dataChannel) return;
    rtc::CopyOnWriteBuffer binBuffer = rtc::CopyOnWriteBuffer((unsigned char*)data, size);
    _dataChannel->Send(webrtc::DataBuffer(binBuffer, true));
}

void PeerClient::OnMessage(const webrtc::DataBuffer& buffer)
{

    //std::string message = std::string(data, buffer.size);
    //std::string(reinterpret_cast<const char*>(buffer.data.data()), buffer.size);
    const char* data = buffer.data.data<char>();
    if (buffer.binary)
    {
        onData.emit(_sid, (void*)data, buffer.size());
    }
    else
    {
        std::string message(data, buffer.size());
        ILOG("Message: {}", message);
    }
}

//void PeerClient::stop()
//{
//    _pc = nullptr;
//}

rtc::Thread* PeerClient::eventHandler()
{
    return RTCEngine::instance()->getPeerEventHandler();
}

void PeerClient::addStreams()
{
    if (!_pc->GetSenders().empty())
    {
        return;  // Already added tracks.
    }

    auto track = RTCEngine::instance()->FetchVideoTrack(_localVideoName);
    if (track)
    {
        auto result_or_error3 = _pc->AddTrack(track, { "stream_id" });
        if (!result_or_error3.ok())
        {
            WLOG("Failed to add video track to PeerConnection: {}", result_or_error3.error().message());
        }
    }
}

void PeerClient::handleSignal(const Json::Value& msg)
{
    if (!_pc) return;
    std::string msgType = msg["type"].asString();
    if ("jsep" == msgType)
    {
        std::unique_ptr<webrtc::SessionDescriptionInterface> sdp = convertToSdp(msg);
        webrtc::SdpType sdpType = sdp->GetType();
        if (sdpType != webrtc::SdpType::kOffer && _role == PEER_ROLE_PUBLISHER)
        {
            _pc->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), sdp.release());
        }
        if (sdpType == webrtc::SdpType::kOffer && _role == PEER_ROLE_SUBSCRIBER)
        {
            _pc->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), sdp.release());
            _pc->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
        }
        
    }
    else if ("candidate" == msgType)
    {
        std::unique_ptr<webrtc::IceCandidateInterface> candidate = convertToCandidate(msg);
        if (!_pc->AddIceCandidate(candidate.release()))
        {
            WLOG("Failed to apply the received candidate");
        }
    }
}

std::unique_ptr<webrtc::SessionDescriptionInterface> PeerClient::convertToSdp(const Json::Value& msg)
{
    std::string sdpStr = msg["sdp"].asString();
    std::string sdpTypeStr = msg["sdpType"].asString();
    absl::optional<webrtc::SdpType> type_maybe = webrtc::SdpTypeFromString(sdpTypeStr);
    if (!type_maybe)
    {
        WLOG("Unknown SDP type: {}", sdpTypeStr);
        return nullptr;
    }
    webrtc::SdpType sdpType = *type_maybe;
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> sdp = webrtc::CreateSessionDescription(sdpType, sdpStr, &error);
    if (!sdp)
    {
        WLOG("Can't parse received session description message. SdpParseError was: {}", error.description);
    }
    return sdp;
}

std::unique_ptr<webrtc::IceCandidateInterface> PeerClient::convertToCandidate(const Json::Value& msg)
{
    std::string sdp_mid = msg["sdpMid"].asString();
    int sdp_mlineindex = msg["sdpMLineIndex"].asInt();
    std::string sdp = msg["candidate"].asString();
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate) {
        WLOG("Can't parse received candidate message. SdpParseError was: {}", error.description);
    }
    return candidate;
}

void PeerClient::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver, const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    webrtc::MediaStreamTrackInterface* trk = receiver->track().get();
    if (NULL == trk) return;
    if (trk->kind() == webrtc::MediaStreamTrackInterface::kAudioKind && _remoteAudioSink)
    {
        ((webrtc::AudioTrackInterface*)trk)->AddSink(_remoteAudioSink);
    }
    else if (trk->kind() == webrtc::MediaStreamTrackInterface::kVideoKind)
    {
        RTCEngine::instance()->addVideoTrack(getSid(), (webrtc::VideoTrackInterface*)trk);
    }
    //auto transceivers = _pc->GetTransceivers();
    //for (const auto& t : transceivers)
    //{
    //    WLOG("direct: " << t->direction() << " kind: " << t->media_type());
    //}
}

void PeerClient::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    webrtc::MediaStreamTrackInterface* trk = receiver->track().get();
    if (NULL == trk) return;
    if (trk->kind() == webrtc::MediaStreamTrackInterface::kAudioKind && _remoteAudioSink)
    {
        ((webrtc::AudioTrackInterface*)trk)->RemoveSink(_remoteAudioSink);
    }
    else if (trk->kind() == webrtc::MediaStreamTrackInterface::kVideoKind)
    {
        RTCEngine::instance()->removeVideoTrack(getSid());
    }
}

void PeerClient::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    std::string sdp;
    if(_loopback)
    {
        if (!_pc->AddIceCandidate(candidate))
        {
            WLOG("Failed to apply the received candidate");
        }
        return;
    }
    if (!candidate->ToString(&sdp))
    {
        WLOG("Failed to serialize candidate");
        return;
    }
    
    Json::Value msg;
    msg["type"] = "candidate";
    msg["role"] = _role;
    msg["sdpMid"] = candidate->sdp_mid();
    msg["sdpMLineIndex"] = candidate->sdp_mline_index();
    msg["candidate"] = sdp;
    sendMessage(MSG_SIGNAL_SEND, msg);
}

void PeerClient::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
    _pc->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

    std::string sdp;
    if (!desc->ToString(&sdp))
    {
        ELOG("Failed to serialize jsep");
        return;
    }
    if(_loopback)
    {
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
        _pc->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), session_description.release());
        return;
    }

    Json::Value msg;
    msg["type"] = "jsep";
    msg["role"] = _role;
    msg["sdpType"] = webrtc::SdpTypeToString(desc->GetType());
    msg["sdp"] = sdp;
    sendMessage(MSG_SIGNAL_SEND, msg);
}

void PeerClient::OnFailure(webrtc::RTCError error)
{
    std::string errType = std::string(webrtc::ToString(error.type()));
    ELOG("CreateSessionDescriptionObserver error. {}, {}", errType, error.message());
}

void PeerClient::sendMessage(int msgId, const Json::Value& msg)
{
    eventHandler()->PostTask(RTC_FROM_HERE, [self = this, msgId, msg]() {
        self->handleMessage(msgId, msg);
    });
}

void PeerClient::handleMessage(int msgId, const Json::Value& msg)
{
    switch (msgId)
    {
    case MSG_SIGNAL_SEND:
        //auto sc = _sc.lock();
        if (_sc) _sc->sendWebrtcSignal(_sid, msg);
        break;
    }
}

webrtc::MediaStreamTrackInterface* PeerClient::getStreamTrack(int type)
{
    //0: local audio; 1: local video; 2: remote audio; 3: remote video
    if (!_pc) return NULL;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders= _pc->GetSenders();
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers = _pc->GetReceivers();
    std::string kind = (type % 2) == 0 ? webrtc::MediaStreamTrackInterface::kAudioKind : webrtc::MediaStreamTrackInterface::kVideoKind;

    webrtc::MediaStreamTrackInterface* track = NULL;
    if (type / 2 == 0)
    {
        for (int i = 0; i < senders.size(); i++)
        {
            auto trk = senders[i]->track();
            if (trk->kind() == kind)
            {
                track = trk.get();
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < receivers.size(); i++)
        {
            auto trk = receivers[i]->track();
            if (trk->kind() == kind)
            {
                track = trk.get();
                break;
            }
        }
    }
    return track;
}

//void PeerClient::setLocalAudioSink(webrtc::AudioTrackSinkInterface* sink)
//{
//    webrtc::AudioTrackInterface* track = (webrtc::AudioTrackInterface*)getStreamTrack(0);
//    if (NULL != track)
//    {
//        if (_localAudioSink) track->RemoveSink(_localAudioSink);
//        if (sink) track->AddSink(sink);
//    }
//    _localAudioSink = sink;
//}

//void PeerClient::setLocalVideoSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
//{
//    webrtc::VideoTrackInterface* track = (webrtc::VideoTrackInterface*)getStreamTrack(1);
//    if (NULL != track )
//    {
//        if (_localVideoSink) track->RemoveSink(_localVideoSink);
//        if (sink) track->AddOrUpdateSink(sink, rtc::VideoSinkWants());
//    }
//    _localVideoSink = sink;
//}

//void PeerClient::setRemoteAudioSink(webrtc::AudioTrackSinkInterface* sink)
//{
//    webrtc::AudioTrackInterface* track = (webrtc::AudioTrackInterface*)getStreamTrack(2);
//    if (NULL != track)
//    {
//        if (_remoteAudioSink) track->RemoveSink(_remoteAudioSink);
//        if (sink) track->AddSink(sink);
//    }
//    _remoteAudioSink = sink;
//}

//void PeerClient::setRemoteVideoSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
//{
//    webrtc::VideoTrackInterface* track = (webrtc::VideoTrackInterface*)getStreamTrack(3);
//    if (NULL != track)
//    {
//        if (_remoteVideoSink) track->RemoveSink(_remoteVideoSink);
//        if (sink) track->AddOrUpdateSink(sink, rtc::VideoSinkWants());
//    }
//    _remoteVideoSink = sink;
//}

void PeerClient::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
    if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected)
    {
        //eventHandler()->PostTask(RTC_FROM_HERE, [self = this]() {
        //    self->_sc->onPeerClosed(self->_sid);
        //}
        _sc->onPeerClosed(_sid);
        ILOG("Peer disconnected {}", new_state);
    }
}

//sigslot::signal1<stVideoFrame>* PeerClient::LocalVideoFrameSignal()
//{
//    return &_localRender->videoFrameSig;
//}
//
//sigslot::signal1<stVideoFrame>* PeerClient::RemoteVideoFrameSignal()
//{
//    return &_remoteRender->videoFrameSig;
//}

void PeerClient::muteVideo(bool mute)
{
    if (!_pc) return;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = _pc->GetSenders();
    const std::string kind = webrtc::MediaStreamTrackInterface::kVideoKind;
    ILOG("222 mute={} senders={}", mute, senders.size());
    for (int i = 0; i < senders.size(); i++)
    {
        auto sender = senders[i];
        if (sender && sender->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO)
        {
            if (mute) sender->SetTrack(nullptr);
            else
            {
                auto track = RTCEngine::instance()->FetchVideoTrack(_localVideoName);
                sender->SetTrack(track);
            }
        }

    }
    //trk->set_enabled(mute);      本地图像显示为黑屏，远端也会显示为黑屏
    //_pc->RemoveTrackNew(sender); 从sender中删除掉track，停止；彻底移除；好像没有完全删除sender
    //sender->SetTrack(nullptr);   可以暂停发送数据
}

void PeerClient::muteVideo2(bool mute)
{
    if (!_pc) return;
    
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = _pc->GetSenders();
    //webrtc::MediaStreamTrackInterface::kAudioKind : webrtc::MediaStreamTrackInterface::kVideoKind;

    const std::string kind = webrtc::MediaStreamTrackInterface::kVideoKind;
    ILOG("mute={} senders={}", mute, senders.size());
    for (int i = 0; i < senders.size(); i++)
    {
        auto sender = senders[i];
        if (sender && sender->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO && mute)
        {
            auto err = _pc->RemoveTrackNew(sender);
            ILOG("err={}", err.ok());
            senders.erase(senders.begin() + i);
        }
        //senders[i]->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO;
        //auto trk = senders[i]->track();
        //if (!trk) continue;
        //if (trk->kind() == kind)
        //{
        //    senders.erase(senders.begin() + i);
        //    //senders.re
        //    if (mute)
        //    {
        //        //_pc->RemoveTrack(senders[i]);
        //        _pc->RemoveTrackNew(senders[i]);
        //    }
        //    
        //    //rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track = trk;
        //    //video_track->SetState(webrtc::MediaStreamTrackInterface::TrackState::kEnded);
        //    //trk->set_enabled(mute);
        //    break;
        //}
        
    }
    if (!mute)
    {
        addStreams();
    }
}

void PeerClient::setLocalVideo(const std::string& video)
{
    _localVideoName = video;
}

}