#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include <ctime>
#include <string>

#define LOCAL

#ifdef LOCAL
	#include "MockUDPClientWrapper.h"
	#include "MockTCPControlSocket.h"
#else
	#include "UDPClientWrapper.hpp"
	#include "TCPControlSocket.hpp"
#endif

#include "TrainingHandler.h"

#define LOG(x) std::cout << x << std::endl

/*
If LOCAL environment variable is active
program initializes MOCK versions of Client and Pi variables
This means no external connection is required
else if LOCAL is not defined
program initializes non-MOCK version of Client and Pi variables
An external connection is made to the droid
*/
#ifdef LOCAL
	//LOG("Running LOCAL processes");
	MockTCPControlSocket *Pi = new MockTCPControlSocket();
	MockUDPClientWrapper *Client = new MockUDPClientWrapper();
#else
	LOG("Running GLOBAL processes");
	TCPControlSocket *Pi = new TCPControlSocket("192.168.43.175", 5001);
	Pi->setup();

	UDPClientWrapper *Client = new UDPClientWrapper("192.168.43.175", 65507, 5000);
	Client->setup();
#endif

int throttle = 0;	// 0 = stopped, 100 = moving (at the slowest possible speed, for now)
int angle = 75; // 30 to 130 degrees, defaults at 75 (straight)
int secondsToRecord = 5;

Mat frame;

void controls() {
	cvWaitKey(1);

	// some real basic controls - change to xbox gamepad? 
	if (GetAsyncKeyState(VK_UP) & 0x8000){
		// the 'Up' arrow key is currently being held down
		throttle = 101;
		printf("Forward pressed!\n");
	}else{
		throttle = 0;
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000){
		// the 'Left' key is currently being held down
		if (angle > 40){
			angle -= 10;
			printf("Left pressed!\n");
		}
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000){
		// the 'Right' key is currently being held down
		if (angle < 120){
			angle += 10;
			printf("Right pressed!\n");
		}
	}
}

void step() {
	// Action the controls
	controls(); 

	// Send to pi
	std::string instruction = ":" + std::to_string(angle) + "," + std::to_string(throttle) + ".";
	std::string str;
	char * writable = new char[str.size() + 1];
	std::copy(instruction.begin(), instruction.end(), writable);
	writable[instruction.size()] = '\0';
	Pi->sendData(writable);
}


void training_run(std::string directory, std::string name, int seconds) {
	TrainingHandler training(directory, name);
	std::time_t startTime = std::time(0);
	std::time_t currentTime = std::time(0);

	while (currentTime - startTime < seconds) {
		frame = Client->receive();
		training.addSample(frame, angle, throttle);
		currentTime = std::time(0);
	}
	training.saveSamples();
}


int main(){
	training_run("C:/Users/Matthew Lee/Desktop/training_data", "training.csv", secondsToRecord);
	return 0;
}
