#include "Spacebrew.h"

bool Spacebrew::m_bOpen = false;
bool Spacebrew::m_bSendConfig = false;

Spacebrew::OnSBOpen Spacebrew::_onOpen = NULL;
Spacebrew::OnSBClose Spacebrew::_onClose = NULL;
Spacebrew::OnSBError Spacebrew::_onError = NULL;

Spacebrew::OnBooleanMessage Spacebrew::_onBooleanMessage = NULL;
Spacebrew::OnRangeMessage Spacebrew::_onRangeMessage = NULL;
Spacebrew::OnStringMessage Spacebrew::_onStringMessage = NULL;
Spacebrew::OnStringMessage Spacebrew::_onOtherMessage = NULL;

Spacebrew::Spacebrew(){
	m_bOpen = m_bSendConfig = false;
}

void Spacebrew::addPublish(char* name, char* type, char* defaultValue){
	auto* p = new PublisherNode();

	p->name = cloneString(name);
	p->type = cloneString(type);
	p->defaultValue = cloneString(defaultValue);

	publishers.push_back(p);
}

void Spacebrew::addPublish(char* name, int defaultValue){
	char sVal[5];
	intToString(defaultValue, (char*)&sVal);
	addPublish(name, "range", (char*)&sVal);
}

void Spacebrew::addSubscribe(char* name, char* type){
	SubscriberNode *s = new SubscriberNode();
	s->name = cloneString(name);
	s->type = cloneString(type);
	subscribers.push_back(s);
}

void Spacebrew::connect(char hostname[], char* clientName, char* description, int port){
	m_hostname = hostname;
	m_port = port;
	m_sClientName = clientName;
	m_sDescription = description;

	Serial.print("[Spacebrew] Connecting to ");
	Serial.print(hostname);
	Serial.print(":");
	Serial.print(port);
	Serial.println("...");

	m_webSocketClient.begin(hostname, port);
	m_webSocketClient.onEvent(std::bind(&Spacebrew::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

}

void Spacebrew::disconnect(){
	m_webSocketClient.disconnect();
}

void Spacebrew::sendConfig(){
//	StaticJsonBuffer<1024> jsonBuffer;
	DynamicJsonBuffer jsonBuffer;

	// Create root object
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& config = root.createNestedObject("config");
	config["name"] = m_sClientName;
	config["description"] = m_sDescription;

	// Create object for publishers
	JsonObject& publish = config.createNestedObject("publish");
	JsonArray& publishMessages = publish.createNestedArray("messages");

	for (auto& p : publishers) {
		JsonObject& node = publishMessages.createNestedObject();
		node["name"] = p->name;
		node["type"] = p->type;
		node["default"] = p->defaultValue;
	}

	// Create object for subscribers
	JsonObject& subscribe = config.createNestedObject("subscribe");
	JsonArray& subscribeMessages = subscribe.createNestedArray("messages");

	for (auto& s : subscribers) {
		JsonObject& node = subscribeMessages.createNestedObject();
		node["name"] = s->name;
		node["type"] = s->type;
	}

	char buffer[1024];
	root.printTo(buffer, sizeof(buffer));

	Serial.print("send: ");
	Serial.println(buffer);
	m_webSocketClient.sendTXT(buffer);

}

void Spacebrew::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_CONNECTED:
			onWSOpen();
			break;
		case WStype_DISCONNECTED:
			onWSClose();
			break;
		case WStype_TEXT:
			onWSMessage((const char*) payload);
			break;
		case WStype_ERROR:
			onWSError((const char*) payload);
			break;
	}
}

void Spacebrew::onWSOpen(){
	m_bOpen = true;
	m_bSendConfig = true;
	Serial.println("[Spacebrew] Connected! ");

	if (_onOpen != NULL){
		_onOpen();
	}
}

void Spacebrew::onWSClose(){
	if (m_bOpen) {
		m_bOpen = false;
		Serial.println("[Spacebrew] Disconnected! ");

		if (_onClose != NULL){
			_onClose();
		}
	}
}

void Spacebrew::onWSError(const char* message){
	Serial.print("[Spacebrew] Error! ");
	Serial.println(message);

	if (_onError != NULL){
		_onError(message);
	}
}

void Spacebrew::onWSMessage(const char* message){
	Serial.print("recv: ");
	Serial.println(message);

//	StaticJsonBuffer<1024> jsonBuffer;
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(message);

	if (!root.success()) {
		Serial.println("[Spacebrew] invalid json message!");
		return;
	}

	const char* name = root["message"]["name"];
	const char* type = root["message"]["type"];
	const char* value = root["message"]["value"];

//	Serial.println(name);
//	Serial.println(type);
//	Serial.println(value);

	if (strcmp((char*) type, "boolean") == 0){
		if (_onBooleanMessage != NULL){
			_onBooleanMessage(name, strcmp(value, (const char*) "true") == 0);
		}
	} else if (strcmp(type, "string") == 0){
		if (_onStringMessage != NULL){
			_onStringMessage(name, value);
		}
	} else if (strcmp(type, "range") == 0){
		if (_onRangeMessage != NULL){
			_onRangeMessage(name, stringToInt((char*) value));
		}
	} else {
		if (_onOtherMessage != NULL){
			_onOtherMessage(name, value);
		}
	}
}

bool Spacebrew::send(char* name, int value){
	char sVal[5];
	intToString(value, (char*)&sVal);
	send(name, (char*)"range", (char*)&sVal);
}

bool Spacebrew::send(char *name, char *type, char *value){
//	StaticJsonBuffer<1024> jsonBuffer;
	DynamicJsonBuffer jsonBuffer;

	// Create root object
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& message = root.createNestedObject("message");
	message["clientName"] = m_sClientName;
	message["name"] = name;
	message["type"] = type;
	message["value"] = value;

	char buffer[1024];
	root.printTo(buffer, sizeof(buffer));

	Serial.print("send: ");
	Serial.println(buffer);
	m_webSocketClient.sendTXT(buffer);
}

void Spacebrew::monitor(){
	if (m_bSendConfig){
		m_bSendConfig = false;
		sendConfig();
	}

	m_webSocketClient.loop();
}

void Spacebrew::onOpen(OnSBOpen function){
	_onOpen = function;
}

void Spacebrew::onClose(OnSBClose function){
	_onClose = function;
}

void Spacebrew::onRangeMessage(OnRangeMessage function){
	_onRangeMessage = function;
}

void Spacebrew::onStringMessage(OnStringMessage function){
	_onStringMessage = function;
}

void Spacebrew::onBooleanMessage(OnBooleanMessage function){
	_onBooleanMessage = function;
}

void Spacebrew::onOtherMessage(OnStringMessage function){
	_onOtherMessage = function;
}

void Spacebrew::onError(OnSBError function){
	_onError = function;
}
