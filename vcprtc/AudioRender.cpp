#include "AudioRender.h"
#include <rtc_base/logging.h>
#include <rtc_base/ref_counted_object.h>

namespace vcprtc {
rtc::scoped_refptr<AudioRender> AudioRender::Create(const std::string& name, std::weak_ptr<AudioFrameObserver> obs)
{
	return new rtc::RefCountedObject<AudioRender>(name, obs);
}

AudioRender::AudioRender(const std::string& name, std::weak_ptr<AudioFrameObserver> obs) : _name(name), _observer(obs)
{

}

AudioRender::~AudioRender()
{

}

void AudioRender::OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames)
{
    RTC_LOG(INFO) << __FUNCTION__ << "AudioRender OnData " << _name;
	if (auto obs = _observer.lock())
	{
		obs->OnData(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
	}
}

}