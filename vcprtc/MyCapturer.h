#pragma once
/**
 *¡¡WindowsÆÁÄ»Â¼ÏñÄ£¿é
 */

#include <api/scoped_refptr.h>
#include <api/video/i420_buffer.h>
#include <modules/desktop_capture/desktop_capturer.h>
#include <modules/desktop_capture/desktop_frame.h>
#include "rtc_base/thread.h"
#include "media/base/adapted_video_track_source.h"
#include "rtc_base/message_handler.h"
#include "test/vcm_capturer.h"
#include "pc/video_track_source.h"
#include <modules/video_capture/video_capture_factory.h>

namespace vcprtc {
class MyCapturer : public rtc::AdaptedVideoTrackSource,
                   //public rtc::MessageHandler,
                   public webrtc::DesktopCapturer::Callback {
 public:
  MyCapturer();
  ~MyCapturer();

  void startCapturer();

  //void CaptureFrame();

  bool is_screencast() const override;

  absl::optional<bool> needs_denoising() const override;

  webrtc::MediaSourceInterface::SourceState state() const override;

  bool remote() const override;

  void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                               std::unique_ptr<webrtc::DesktopFrame> frame) override;
  //void OnMessage(rtc::Message* msg) override;

  void run();

private:
    std::unique_ptr<rtc::Thread> _eventHandlerThread;
    std::unique_ptr<webrtc::DesktopCapturer> capturer_;
    rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
    //mutable volatile int ref_count_;
    bool _running;
};

class MyCapturerTrackSource : public webrtc::VideoTrackSource {
public:
    static rtc::scoped_refptr<MyCapturerTrackSource> Create() {
        const size_t kWidth = 640;
        const size_t kHeight = 480;
        const size_t kFps = 30;
        std::unique_ptr<webrtc::test::VcmCapturer> capturer;
        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
            webrtc::VideoCaptureFactory::CreateDeviceInfo());
        if (!info) {
            return nullptr;
        }
        int num_devices = info->NumberOfDevices();
        for (int i = 0; i < num_devices; ++i) {
            capturer = absl::WrapUnique(
                webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
            if (capturer) {
                return new
                    rtc::RefCountedObject<MyCapturerTrackSource>(std::move(capturer));
            }
        }

        return nullptr;
    }
    void startCapturer() { ; }

protected:
    explicit MyCapturerTrackSource(
        std::unique_ptr<webrtc::test::VcmCapturer> capturer)
        : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

private:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
        return capturer_.get();
    }
    std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};
}
