#include "application.h"
#include "OSC/OSCMessage.h"
#include "OSC/OSCBundle.h"

//----- OUTPUTS
SYSTEM_MODE(SEMI_AUTOMATIC);

//----- REGISTERED OSC COMMANDS
char OscCmd_TestSendMsg[13] = "/sendTestMsg";			// 12 characters + 1 for the "null-terminated string" -> '\0'

//----- IP ADRESSES
IPAddress computerIPAddress = IPAddress(192,168,40,134);	// put the IP address of your computer here
IPAddress coreIPAddress;
OSCMessage coreIPMessage("/coreip");

//----- PORTS
#define LOCALPORT  8888		// to send data to the Spark Core (from the computer)
#define REMOTEPORT 9999		// to send data to the computer (from here)

//----- UDP + overloading the inappropriate UDP functions of the Spark Core (REQUIRED !)
class myUDP : public UDP {
private :
	uint8_t myBuffer[512];
	int offset = 0;
public :
	virtual int beginPacket(IPAddress ip, uint16_t port){
		offset = 0;
		return UDP::beginPacket(ip, port);
	};
	virtual int endPacket(){
		return UDP::write(myBuffer, offset);
	};
	virtual size_t write(uint8_t buffer) {
		write(&buffer, 1);
		return 1;
	}
	virtual size_t write(const uint8_t *buffer, size_t size) {
		memcpy(&myBuffer[offset], buffer, size);
		offset += size;
		return size;
	}
};

myUDP Udp;


//=========================================================================================
//=========================================================================================
void setup()
{
	WiFi.on();

	/*
	if (WiFi.hasCredentials()) {
		WiFi.clearCredentials();
	}

	WiFi.setCredentials("test_network" , "test_network_password");
	*/

	WiFi.connect();

	if (Spark.connected() == false) {
		Spark.connect();
	}

	// Start UDP
	Udp.begin(LOCALPORT);

	// Get the IP address of the Spark Core and send it as an OSC Message
	coreIPAddress = WiFi.localIP();
	coreIPMessage.add(coreIPAddress[0]).add(coreIPAddress[1]).add(coreIPAddress[2]).add(coreIPAddress[3]);

	Udp.beginPacket(computerIPAddress, REMOTEPORT);
	coreIPMessage.send(Udp);
	Udp.endPacket();
}

//=========================================================================================
//===== TEST sending an OSCMessage (called when the Spark Core receives "/sendTestMsg" as an OSC Message)
//=========================================================================================
void sendOSCTestMsg(OSCMessage &mess)
{
	OSCMessage testMsg_toSend("/testmessage");
	testMsg_toSend.add((float)2.6).add(150).add("hahaha").add(-1000);

	Udp.beginPacket(computerIPAddress, REMOTEPORT);
	testMsg_toSend.send(Udp); // send the bytes
	Udp.endPacket();
	testMsg_toSend.empty(); // empty the message to free room for a new one
}

//=========================================================================================
void loop()
{
		OSCMessage testMsg_Received;

		int bytesToRead = Udp.parsePacket();	// how many bytes are available via UDP
		if (bytesToRead > 0) {
			while(bytesToRead--) {
				testMsg_Received.fill(Udp.read());	// filling the OSCMessage with the incoming data
			}
			if(!testMsg_Received.hasError()) { // if the address corresponds to a command, we dispatch it to the corresponding function
				testMsg_Received.dispatch(OscCmd_TestSendMsg , sendOSCTestMsg);
			}
		}
}
