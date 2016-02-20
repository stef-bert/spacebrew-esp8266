#include "Spacebrew.h"

bool Spacebrew::m_bOpen = false;
bool Spacebrew::m_bSendConfig = false;
Spacebrew::OnBooleanMessage Spacebrew::_onBooleanMessage = NULL;
Spacebrew::OnRangeMessage Spacebrew::_onRangeMessage = NULL;
Spacebrew::OnStringMessage Spacebrew::_onStringMessage = NULL;
Spacebrew::OnStringMessage Spacebrew::_onOtherMessage = NULL;
Spacebrew::OnSBOpen Spacebrew::_onOpen = NULL;
Spacebrew::OnSBClose Spacebrew::_onClose = NULL;
Spacebrew::OnSBError Spacebrew::_onError = NULL;

Spacebrew::Spacebrew(){
	m_bOpen = m_bSendConfig = false;
}

void Spacebrew::sendConfig(){
	const char *c1 = "{\"config\":{\"name\":\"",
	*c2 = "\",\"description\":\"",
	*c3 = "\",\"publish\":{\"messages\":[",
	*c4 = "{\"name\":\"",
	*c5 = "\",\"type\":\"",
	*c6 = "\"}",
	*c7 = ",",
	*c8 = "]},\"subscribe\":{\"messages\":[",
	*c9 = "]}}}",
	*c10 = "\",\"default\":\"";
	int len = strlen(c1)+strlen(c2)+strlen(c3)+strlen(c8)+strlen(c9)+strlen(m_sClientName)+strlen(m_sDescription);

	PublisherNode *p = publishers;
	while(p != NULL){
		len += strlen(c4)+strlen(c5)+strlen(c6)+strlen(c7)+strlen(p->name)+strlen(p->type)+strlen(c10)+strlen(p->defaultValue);
		p = p->next;
	}
	SubscriberNode *s = subscribers;
	while(s != NULL){
		len += strlen(c4)+strlen(c5)+strlen(c6)+strlen(c7)+strlen(s->name)+strlen(s->type);
		s = s->next;
	}

	char b[len+1];
	strcpy(b, c1);
	strcat(b, m_sClientName);
	strcat(b, c2);
	strcat(b, m_sDescription);
	strcat(b, c3);
	p = publishers;
	while(p != NULL){
		strcat(b, c4);
		strcat(b, p->name);
		strcat(b, c5);
		strcat(b, p->type);
		strcat(b, c10);
		strcat(b, p->defaultValue);
		strcat(b, c6);
		p = p->next;
		if (p != NULL){
			strcat(b, c7);
		}
	}
	strcat(b, c8);
	s = subscribers;
	while(s != NULL){
		strcat(b, c4);
		strcat(b, s->name);
		strcat(b, c5);
		strcat(b, s->type);
		strcat(b, c6);
		s = s->next;
		if (s != NULL){
			strcat(b, c7);
		}
	}
	strcat(b, c9);
	m_webSocketClient.sendData(b);
}

void Spacebrew::connect(WiFiClient* wifiClient, char hostname[], char* clientName, char* description, int port){
	m_wifiClient = wifiClient;
	m_hostname = hostname;
	m_port = port;

	// WiFi connection
	if (m_wifiClient->connect(hostname, port)) {
		Serial.println("WiFi connected");;
	} else {
		Serial.println("WiFi failed to connect");;
		while (1) { }
	}

	// Websocket handshake
	m_webSocketClient.path = "/";
	m_webSocketClient.host = hostname;

	if (m_webSocketClient.handshake(*m_wifiClient)) {
		Serial.println("Websocket handshake successful");
		m_sClientName = clientName;
		m_sDescription = description;
		onWSOpen();
	} else {
		Serial.println("Websocket handshake failed");
		while(1) {
			// Hang on failure
		}
	}
}

void Spacebrew::addPublish(char* name, char* type, char* defaultValue){
	PublisherNode *p = new PublisherNode();
	p->name = cloneString(name);
	p->type = cloneString(type);
	p->defaultValue = cloneString(defaultValue);
	if (publishers == NULL){
		publishers = p;
	} else {
		PublisherNode *curr = publishers;
		while(curr->next != NULL){
			curr = curr->next;
		}
		curr->next = p;
	}
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
	if (subscribers == NULL){
		subscribers = s;
	} else {
		SubscriberNode *curr = subscribers;
		while(curr->next != NULL){
			curr = curr->next;
		}
		curr->next = s;
	}
}
void Spacebrew::disconnect(){
	onWSClose();
}

void Spacebrew::onWSMessage(char* message){
	//Serial.println(message);
	const char *i1 = "{\"message\":{\"name\":\"",
	*i2 = "\",\"type\":\"",
	*i3 = "\",\"value\":\"";
	char *name, *type, *value;
	name = message + strlen(i1);
	message = strchr(name, '"');
	*message = NULL;
	type = message + strlen(i2);
	message = strchr(type, '"');
	*message = NULL;
	value = message + strlen(i3);
	if (*(value - 1) == '"'){
		message = strchr(value, '"');
	} else {
		value--;
		message--;
		message = strchr(value, '}');
	}
	*message = NULL;
	if (strcmp(type, "boolean") == 0){
		if (_onBooleanMessage != NULL){
			_onBooleanMessage(name, strcmp(value, "true") == 0);
		}
	} else if (strcmp(type, "string") == 0){
		if (_onStringMessage != NULL){
			_onStringMessage(name, value);
		}
	} else if (strcmp(type, "range") == 0){
		if (_onRangeMessage != NULL){
			_onRangeMessage(name, stringToInt(value));
		}
	} else {
		if (_onOtherMessage != NULL){
			_onOtherMessage(name, value);
		}
	}
}
bool Spacebrew::send(char *name, char *type, char *value){
	unsigned long begin = millis();
	const char *m1 = "{\"message\":{\"clientName\":\"",
	*m2 = "\",\"name\":\"",
	*m3 = "\",\"type\":\"",
	*m4 = "\",\"value\":\"",
	*m5 = "\"}}";
	int len = strlen(m1)+strlen(m2)+strlen(m3)+strlen(m4)+strlen(m5)+strlen(m_sClientName)+strlen(name)+strlen(type)+strlen(value);
	char b[len+1];
	strcpy(b, m1);
	strcat(b, m_sClientName);
	strcat(b, m2);
	strcat(b, name);
	strcat(b, m3);
	strcat(b, type);
	strcat(b, m4);
	strcat(b, value);
	strcat(b, m5);
	unsigned long end = millis();
	m_webSocketClient.sendData(b);
	unsigned long after = millis();
	int stringTime = end - begin;
	int sendTime = after - end;
	Serial.print("Send: string time ");
	Serial.print(stringTime);
	Serial.print(", send time ");
	Serial.println(sendTime);
}
bool Spacebrew::send(char* name, int value){
	char sVal[5];
	intToString(value, (char*)&sVal);
	send(name, (char*)"range", (char*)&sVal);
}
void Spacebrew::onInternalError(char *message){
	if (_onError != NULL){
		_onError(message);
	}
}
void Spacebrew::monitor(){
	if (!m_bOpen){
		return;
	}

	if (!m_wifiClient->connected()) {
		onWSClose();
		connect(m_wifiClient, m_hostname, m_sClientName, m_sDescription, m_port);
		return;
	}

	if (m_bSendConfig){
		m_bSendConfig = false;
		sendConfig();
	}

	String data;
	m_webSocketClient.getData(data);
	if (data.length() > 0) {
		Serial.println("recv: " + data);
		if (data == "ping") {
			// umm...respond to ping?
		} else {
			onWSMessage((char*) data.c_str());
		}
	}
}

void Spacebrew::onWSOpen(){
	m_bOpen = true;
	m_bSendConfig = true;
	if (_onOpen != NULL){
		_onOpen();
	}
}

void Spacebrew::onWSClose(){
	m_bOpen = false;
	if (_onClose != NULL){
		_onClose();
	}
}

void Spacebrew::onWSError(char* message){
	if (_onError != NULL){
		_onError(message);
	}
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
