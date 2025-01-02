#include "VideoRender.h"
#include <rtc_base/logging.h>
#include <rtc_base/ref_counted_object.h>
#include <api/video/i420_buffer.h>
#include <chrono>
#include "Observable.h"
#include "logger/logger.h"
//#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace vcprtc {
rtc::scoped_refptr<VideoRender> VideoRender::Create(const std::string& name)
{
	return new rtc::RefCountedObject<VideoRender>(name);
}

VideoRender::VideoRender(const std::string& name):
	_name(name)
{
	_lastRecTime = -1;
	_frameCount = 0;
}

VideoRender::~VideoRender()
{
}

long long getCurrentTimeMillis() 
{
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
}

void VideoRender::OnFrame(const webrtc::VideoFrame& frame)
{
	long long current = getCurrentTimeMillis();
	_frameCount++;
	if (_lastRecTime <= 0) _lastRecTime = current;
	if (current - _lastRecTime >= 3000)
	{
		double fps = 1000.0 * _frameCount / (current - _lastRecTime);
		_frameCount = 0;
		_lastRecTime = current;
		ILOG("{} {}x{} {:.2f}fps", _name, frame.width(), frame.height(), fps);
	}
	
	rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
	if (frame.rotation() != webrtc::kVideoRotation_0)
	{
		buffer = webrtc::I420Buffer::Rotate(*buffer, frame.rotation());
	}

	//webrtc::ConvertFromI420(frame, webrtc::VideoType::kARGB, 0, _rgba);

	stVideoFrame frame0;
	frame0.width = buffer->width();
	frame0.height = buffer->height();
	frame0.format = 0;
	frame0.pixes[0] = buffer->DataY();
	frame0.pixes[1] = buffer->DataU();
	frame0.pixes[2] = buffer->DataV();
	frame0.pixes[3] = NULL;// _rgba;
	frame0.stride[0] = buffer->StrideY();
	frame0.stride[1] = buffer->StrideU();
	frame0.stride[2] = buffer->StrideV();
	frame0.stride[3] = 0;
	videoFrameSig.emit(frame0);
}


}