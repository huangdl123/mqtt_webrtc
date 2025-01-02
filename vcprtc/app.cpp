#include "app.h"
#include "CallApp.h"
#include "RtcEngine.h"

namespace vcprtc {
std::shared_ptr<IApp> IApp::create(const std::string& clientId)
{
	std::shared_ptr<CallApp> instance = std::make_shared<CallApp>(clientId);
	return instance;
}

std::shared_ptr<IRTCEngine> IRTCEngine::instance()
{
	return RTCEngine::instance();
}

}