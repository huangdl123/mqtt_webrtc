#pragma once
#include <mutex>
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <api/media_stream_interface.h>
#include "app.h"
#include <rtc_base/sigslot_repeater.h>

namespace vcprtc {
class VideoRender : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
	public rtc::RefCountInterface
{
public:
	static rtc::scoped_refptr<VideoRender> Create(const std::string& name);
	void OnFrame(const webrtc::VideoFrame& frame) override;
	sigslot::signal1<stVideoFrame> videoFrameSig;
protected:
	explicit VideoRender(const std::string& name);
	~VideoRender();
private:
	std::string _name;
	long long _lastRecTime; //ms
	int _frameCount;
};

}