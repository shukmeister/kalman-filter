// motion control implementation
// compiled by Ben Shukman

// angles in Quids, 10 bit ADC
// implmenetation: kalman filters, angle calculation, sensor noise reduction, PID control, motor control, remote control

#include <math.h>

#define   GYR_Y                 0                              // Gyro Y (IMU pin #4)
#define   ACC_Z                 1                              // Acc  Z (IMU pin #7)
#define   ACC_X                 2                              // Acc  X (IMU pin #9)         

int sensorValue[3]  = { 0, 0, 0};
int sensorZero[3]   = { 0, 0, 0};

int LOOP_DURATION = 9; // how long each loop is
int lastLoopTime = LOOP_DURATION; // 
int currentLoopTime = LOOP_DURATION; // how many seconds have passed since loop started
unsigned long loopStartTime = 0;


void setup() {
	analogReference(EXTERNAL);                                   // Aref 3.3V
	Serial.begin(115200);
	delay(100);
	calibrateSensors();
}

void loop() {

	// loop timing control
	currentLoopTime = millis() - loopStartTime; 
	if (currentLoopTime < LOOP_DURATION) { // change use of delay to a time ticker when asynchronous processes needed later
			delay(LOOP_DURATION - currentLoopTime);
	}
	lastLoopTime = millis() - loopStartTime;
	loopStartTime = millis();
}

void serialOut_timing() {

	static int skip = 0;
	if (skip++ == 5) { // display every 500 ms (at 100 Hz)
		skip = 0;
		Serial.print(currentLoopTime); Serial.print(",");
		Serial.print(lastLoopTime); Serial.print("\\n");
	}
}

// Set zero sensor values
void calibrateSensors() {      

	long v;
	for(int n=0; n<3; n++) {
		v = 0;
		for(int i=0; i<50; i++) {
			v += readSensor(n);
		}
		sensorZero[n] = v/50;
	}
	sensorZero[ACC_Z] -= 102;
}