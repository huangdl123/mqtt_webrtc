#include "MqttSignalClient.h"
#include "mqtt/async_client.h"
#include "mqtt/topic.h"
#include "logger/logger.h"

namespace vcprtc {
std::string getStringValue(const Json::Value& val, const std::string& key, const std::string& defaultValue = "")
{
	return val.isMember(key) ?  val[key].asString() : defaultValue;
}

int getIntValue(const Json::Value& val, const std::string& key, int defaultValue = 0)
{
	return val.isMember(key) ? val[key].asInt() : defaultValue;
}

MqttSignalClient::MqttSignalClient(const Json::Value& config, ISignalClientObsever* callback) :
	ISignalClient(callback),
	_pubQos(0), 
	_pubRetained(false)
{	
	connect(config);

	if (config.isMember("publish"))
	{
		const Json::Value& jPublish = config["publish"];
		_pubQos = getIntValue(jPublish, "qos");
		_pubRetained = getIntValue(jPublish, "retained") != 0;
	}
}

MqttSignalClient::~MqttSignalClient()
{
	disconnect();
}

int MqttSignalClient::connect(const Json::Value& config)
{
	int ret = 0;
	const std::string server = getStringValue(config, "server", "mqtt://47.97.102.134:11893");
	const std::string clientId = getStringValue(config, "clientId");
	const std::string username = getStringValue(config, "username");
	const std::string password = getStringValue(config, "password");

	_cli = std::make_shared<mqtt::async_client>(server, clientId, mqtt::create_options(MQTTVERSION_5));
	_cli->set_disconnected_handler([this](const mqtt::properties& properties, mqtt::ReasonCode code) {
		onSignalClosed();
	});
	_cli->set_message_callback([this](mqtt::const_message_ptr msg) {
		onSignalMessage(msg->get_payload_str());
	});
	_cli->set_connected_handler([self = this, config](const std::string&) {
		if (!config.isMember("subscribes")) return;
		if (!self->eventThread()) return;
		self->eventThread()->PostTask(RTC_FROM_HERE, [self, config]() {
			self->subcribeTopics(config["subscribes"]);
		});
	});

	auto builder = mqtt::connect_options_builder()
		//.properties({ {mqtt::property::SESSION_EXPIRY_INTERVAL, 604800} })
		.properties({ {mqtt::property::SESSION_EXPIRY_INTERVAL, 0} })
		.user_name(username).password(password)
		.automatic_reconnect(true)
		.clean_start(true)
		.keep_alive_interval(std::chrono::seconds(20));

	if (config.isMember("will") && config["will"].isObject())
	{
		Json::Value jWill = config["will"];
		const std::string willTopic = getStringValue(jWill, "topic", "will");
		const std::string willMsg = getStringValue(jWill, "msg", "");
		int willQos = getIntValue(jWill, "qos", 1);
		bool willRetained = getIntValue(jWill, "retained", 0) != 0;
		builder.will(mqtt::message(willTopic, willMsg, willQos, willRetained));
	}
	auto connOpts = builder.finalize();

	try
	{
		_cli->connect(connOpts)->wait();
	}
	catch (const mqtt::exception& exc)
	{
		std::cerr << "\nERROR: Unable to connect. " << exc.what() << std::endl;
		ret = -1;
	}
	return ret;
}

int MqttSignalClient::disconnect()
{
	int ret = 0;
	try
	{
		_cli->disconnect()->wait();
		_cli = nullptr;
	}
	catch (const mqtt::exception& exc) {
		std::cerr << exc.what() << std::endl;
		ret = -1;
	}
	return ret;
}

int MqttSignalClient::subcribeTopics(const Json::Value& topics)
{
	int ret = 0;
	if (!topics.isArray()) return ret;
	for (int i = 0; i < topics.size(); i++)
	{
		const Json::Value& jTopic = topics[i];
		const std::string topicName = getStringValue(jTopic, "topic", "");
		int qos = getIntValue(jTopic, "qos", 0);
		bool retained = getIntValue(jTopic, "retained", 0) != 0;
		bool noLocal = getIntValue(jTopic, "noLocal", 0) != 0;
		auto subOpts = mqtt::subscribe_options(noLocal);
		mqtt::topic topic(*_cli.get(), topicName, qos, retained);

		try
		{
			topic.subscribe(subOpts)->wait();
		}
		catch (const mqtt::exception& exc)
		{
			std::cerr << "\nERROR: Unable to subscribe topic(" << topicName << ")" << exc.what() << std::endl;
			ret -= 1;
		}
	}
	return ret;
}

void MqttSignalClient::sendText(const std::string& dst, const std::string& text)
{
	mqtt::topic topic(*_cli.get(), dst);
	try
	{
		topic.publish(text, _pubQos, _pubRetained)->wait();
	}
	catch (const mqtt::exception& exc)
	{
		std::cerr << "\nERROR: Unable to subscribe topic(" << dst << ")" << exc.what() << std::endl;
	}
}

void MqttSignalClient::sendJson(const Json::Value& val)
{
	const std::string topicName = getStringValue(val, "topic", "");
	const std::string payload = getStringValue(val, "payload", "");
	int qos = getIntValue(val, "qos", 0);
	bool retained = getIntValue(val, "retained", 0) != 0;
	if ("" == topicName)
	{
		return;
	}

	mqtt::topic topic(*_cli.get(), topicName);
	try
	{
		topic.publish(payload, qos, retained)->wait();
	}
	catch (const mqtt::exception& exc)
	{
	}
}

}