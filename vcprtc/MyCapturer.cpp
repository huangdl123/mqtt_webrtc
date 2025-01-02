#include "MyCapturer.h"
#include "rtc_base/thread.h"
#include <modules/desktop_capture/desktop_capture_options.h>
#include <third_party/libyuv/include/libyuv.h>
#include "logger/logger.h"

namespace vcprtc {
MyCapturer::MyCapturer() {
    _running = false;
    _eventHandlerThread = rtc::Thread::Create();
    _eventHandlerThread->SetName("peerclient", nullptr);
    _eventHandlerThread->Start();
}

MyCapturer::~MyCapturer()
{
    //_eventHandlerThread = nullptr;
    _running = false;
    rtc::Thread::SleepMs(100);
    capturer_ = nullptr;
    _eventHandlerThread = nullptr;
    ILOG("MyCapture released");
}

void MyCapturer::startCapturer() {
  auto options = webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);
  auto capturer0 = webrtc::DesktopCapturer::CreateScreenCapturer(options);
  auto capturer2 = webrtc::DesktopCapturer::CreateWindowCapturer(options);

  webrtc::DesktopCapturer::SourceList list, list2;
  capturer0->GetSourceList(&list);
  ILOG("screen(0) source count: {}", list.size());
  for (int i = 0; i < list.size(); i++)
  {
      auto src = list[i];
      ILOG("source {}: {}", src.id,  src.title);
  }

  capturer2->GetSourceList(&list2);
  ILOG("window(1) source count: {}", list2.size());
  for (int i = 0; i < list2.size(); i++)
  {
      auto src = list2[i];
      ILOG("source {}: {}", src.id, src.title);
  }

  int sourceId = 0;
  int sourceType = 0;
  //scanf_s("%d %d", &sourceType, &sourceId);

  if (sourceType == 0)
  {
      capturer_ = std::move(capturer0);
  }
  else
  {
      capturer_ = std::move(capturer2);
  }
  if(sourceId >= 0) capturer_->SelectSource(sourceId);
    

  capturer_->Start(this);
  //CaptureFrame();

  _eventHandlerThread->PostTask(RTC_FROM_HERE, [self = this]() {
      self->run();
  });
}

webrtc::MediaSourceInterface::SourceState MyCapturer::state() const {
  return webrtc::MediaSourceInterface::kLive;
}

bool MyCapturer::remote() const {
  return false;
}

bool MyCapturer::is_screencast() const {
  return true;
}

absl::optional<bool> MyCapturer::needs_denoising() const {
  return false;
}

void MyCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                 std::unique_ptr<webrtc::DesktopFrame> frame) {
  if (result != webrtc::DesktopCapturer::Result::SUCCESS)
    return;

  int width = frame->size().width();
  int height = frame->size().height();

  if (!i420_buffer_.get() ||
      i420_buffer_->width() * i420_buffer_->height() < width * height) {
    i420_buffer_ = webrtc::I420Buffer::Create(width, height);
  }
  libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                        i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                        i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                        i420_buffer_->StrideV(), 0, 0, width, height, width,
                        height, libyuv::kRotate0, libyuv::FOURCC_ARGB);

  OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
}

void MyCapturer::run()
{
    _running = true;
    while (_running)
    {
        capturer_->CaptureFrame();
        rtc::Thread::SleepMs(33);
    }
}

//void MyCapturer::OnMessage(rtc::Message* msg) {
//  if (msg->message_id == 0)
//    CaptureFrame();
//}

//void MyCapturer::CaptureFrame() {
//  capturer_->CaptureFrame();
//
//  rtc::Location loc(__FUNCTION__, __FILE__);
//  //rtc::Thread::Current()->PostDelayed(loc, 33, this, 0);
//  if(_eventHandlerThread)
//    _eventHandlerThread->PostDelayed(loc, 33, this, 0);
//}
}