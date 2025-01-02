#pragma once
#include <api/media_stream_interface.h>

namespace vcprtc {
class AudioFrameObserver
{
public:
	virtual void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames)=0;
};

class AudioRender : public webrtc::AudioTrackSinkInterface,
    public rtc::RefCountInterface
{
public:
    static rtc::scoped_refptr<AudioRender> Create(const std::string& name, std::weak_ptr<AudioFrameObserver> obs);
    virtual void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) override;
    
protected:
    explicit AudioRender(const std::string& name, std::weak_ptr<AudioFrameObserver> obs);
    ~AudioRender();
private:
    std::string _name;
    std::weak_ptr<AudioFrameObserver> _observer;
};
}