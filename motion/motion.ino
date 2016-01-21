// motion control implementation
// compiled by Ben Shukman

// angles in Quids, 10 bit ADC
// implmenetation: kalman filters, angle calculation, sensor noise reduction, PID control, motor control, remote control

// Main module   K_bot               angles in Quids, 10 bit ADC  -------------
// 4 - Checking sensor data format    display raw sensors data


#include <Wire.h>
#include <Kalman.h> // Source: https://github.com/TKJElectronics/KalmanFilter
// #include <i2c_t3.h>

#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead - please read: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf

Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;

/* IMU Data */
double accX, accY, accZ;
double gyroX, gyroY, gyroZ;
int16_t tempRaw;

double gyroXangle, gyroYangle; // Angle calculate using the gyro only
double compAngleX, compAngleY; // Calculated angle using a complementary filter
double kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

uint32_t timer;
uint8_t i2cData[14]; // Buffer for I2C data

// TODO: Make calibration routine

int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

void setup() {
	Serial.begin(115200);
	Wire.begin();
	#if ARDUINO >= 157
		Wire.setClock(400000UL); // Set I2C frequency to 400kHz
	#else
		TWBR = ((F_CPU / 400000UL) - 16) / 2; // Set I2C frequency to 400kHz
	#endif

	i2cData[0] = 7; // Set the sample rate to 1000Hz - 8kHz/(7+1) = 1000Hz
	i2cData[1] = 0x00; // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering, 8 KHz sampling
	i2cData[2] = 0x00; // Set Gyro Full Scale Range to ±250deg/s
	i2cData[3] = 0x00; // Set Accelerometer Full Scale Range to ±2g
	while (i2cWrite(0x19, i2cData, 4, false)); // Write to all four registers at once
	while (i2cWrite(0x6B, 0x01, true)); // PLL with X axis gyroscope reference and disable sleep mode

	while (i2cRead(0x75, i2cData, 1));
	if (i2cData[0] != 0x68) { // Read "WHO_AM_I" register
		Serial.print(F("Error reading sensor"));
		while (1);
	}

	delay(100); // Wait for sensor to stabilize
	delay(2000);

	/* Set kalman and gyro starting angle */
	while (i2cRead(0x3B, i2cData, 6));
	accX = (i2cData[0] << 8) | i2cData[1];
	accY = (i2cData[2] << 8) | i2cData[3];
	accZ = (i2cData[4] << 8) | i2cData[5];

	// Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
	// atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
	// It is then converted from radians to degrees
	#ifdef RESTRICT_PITCH // Eq. 25 and 26
		double roll  = atan2(accY, accZ) * RAD_TO_DEG;
		double pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
	#else // Eq. 28 and 29
		double roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
		double pitch = atan2(-accX, accZ) * RAD_TO_DEG;
	#endif

	kalmanX.setAngle(roll); // Set starting angle
	kalmanY.setAngle(pitch);
	gyroXangle = roll;
	gyroYangle = pitch;
	compAngleX = roll;
	compAngleY = pitch;

	timer = micros();

	// initialize UART
	// Serial.begin(9600);
	// delay(2000);

	Wire.begin();
	Wire.beginTransmission(0x68);
	Wire.write(0x6B);  // PWR_MGMT_1 register
	Wire.write(0);     // set to zero (wakes up the MPU-6050)
	Wire.endTransmission(true);

	// // setup IMU
	// i2cData[0] = 7; // Set the sample rate to 1000Hz - 8kHz/(7+1) = 1000Hz
	// i2cData[1] = 0x00; // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering, 8 KHz sampling
	// i2cData[2] = 0x00; // Set Gyro Full Scale Range to ±250deg/s
	// i2cData[3] = 0x00; // Set Accelerometer Full Scale Range to ±2g
	// while (i2cWrite(0x19, i2cData, 4, false)); // Write to all four registers at once
	// while (i2cWrite(0x6B, 0x01, true)); // PLL with X axis gyroscope reference and disable sleep mode

	// while (i2cRead(0x75, i2cData, 1));
	// if (i2cData[0] != 0x68) { // Read "WHO_AM_I" register
	// 	Serial.print(F("Error reading sensor"));
	// 	while (1);
	// } else {
	// 	Serial.println("Sensor connection established");
	// }

	// delay(100); // Wait for sensor to stabilize

	// // set kalman and gyro starting angle
	// while (i2cRead(0x3B, i2cData, 6));
	// accX = (i2cData[0] << 8) | i2cData[1];
	// accY = (i2cData[2] << 8) | i2cData[3];
	// accZ = (i2cData[4] << 8) | i2cData[5];
}

// void loop() {
// 	Wire.beginTransmission(0x68);
// 	Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
// 	Wire.endTransmission(false);
// 	Wire.requestFrom(0x68,14,true);  // request a total of 14 registers
// 	AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
// 	AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
// 	AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
// 	Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
// 	GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
// 	GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
// 	GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
// 	Serial.print("AcX = "); Serial.print(AcX);
// 	Serial.print(" | AcY = "); Serial.print(AcY);
// 	Serial.print(" | AcZ = "); Serial.print(AcZ);
// 	Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
// 	Serial.print(" | GyX = "); Serial.print(GyX);
// 	Serial.print(" | GyY = "); Serial.print(GyY);
// 	Serial.print(" | GyZ = "); Serial.println(GyZ);
// 	delay(333);
// }

void loop() {
	// Serial.print(F("i2cRead failed: "));
	// Serial.println("asdf");
	// while (i2cRead(0x3B, i2cData, 14));
	// for (uint8_t i = 0; i < 15; i++) {
	// 	Serial.println(i2cData[i]);
	// }

	while (i2cRead(0x3B, i2cData, 14));
	accX = ((i2cData[0] << 8) | i2cData[1]);
	accY = ((i2cData[2] << 8) | i2cData[3]);
	accZ = ((i2cData[4] << 8) | i2cData[5]);
	tempRaw = (i2cData[6] << 8) | i2cData[7];
	gyroX = (i2cData[8] << 8) | i2cData[9];
	gyroY = (i2cData[10] << 8) | i2cData[11];
	gyroZ = (i2cData[12] << 8) | i2cData[13];

	// Serial.print("AcX = "); Serial.print(accX);
	// Serial.print(" | AcY = "); Serial.print(accY);
	// Serial.print(" | AcZ = "); Serial.print(accZ);
	// Serial.print(" | Tmp = "); Serial.print(tempRaw/340.00+36.53);  //equation for temperature in degrees C from datasheet
	// Serial.print(" | GyX = "); Serial.print(gyroX);
	// Serial.print(" | GyY = "); Serial.print(gyroY);
	// Serial.print(" | GyZ = "); Serial.println(gyroZ);

	double dt = (double)(micros() - timer) / 1000000; // Calculate delta time
	timer = micros();

	#ifdef RESTRICT_PITCH // Eq. 25 and 26
	  double roll  = atan2(accY, accZ) * RAD_TO_DEG;
	  double pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
	#else // Eq. 28 and 29
	  double roll  = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
	  double pitch = atan2(-accX, accZ) * RAD_TO_DEG;
	#endif

	double gyroXrate = gyroX / 131.0; // Convert to deg/s
	double gyroYrate = gyroY / 131.0; // Convert to deg/s

	#ifdef RESTRICT_PITCH
	  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
	  if ((roll < -90 && kalAngleX > 90) || (roll > 90 && kalAngleX < -90)) {
	    kalmanX.setAngle(roll);
	    compAngleX = roll;
	    kalAngleX = roll;
	    gyroXangle = roll;
	  } else
	    kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter

	  if (abs(kalAngleX) > 90)
	    gyroYrate = -gyroYrate; // Invert rate, so it fits the restriced accelerometer reading
	  kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt);
	#else
	  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
	  if ((pitch < -90 && kalAngleY > 90) || (pitch > 90 && kalAngleY < -90)) {
	    kalmanY.setAngle(pitch);
	    compAngleY = pitch;
	    kalAngleY = pitch;
	    gyroYangle = pitch;
	  } else
	    kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt); // Calculate the angle using a Kalman filter

	  if (abs(kalAngleY) > 90)
	    gyroXrate = -gyroXrate; // Invert rate, so it fits the restriced accelerometer reading
	  kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter
	#endif

	  gyroXangle += gyroXrate * dt; // Calculate gyro angle without any filter
	  gyroYangle += gyroYrate * dt;
	  //gyroXangle += kalmanX.getRate() * dt; // Calculate gyro angle using the unbiased rate
	  //gyroYangle += kalmanY.getRate() * dt;

	  compAngleX = 0.93 * (compAngleX + gyroXrate * dt) + 0.07 * roll; // Calculate the angle using a Complimentary filter
	  compAngleY = 0.93 * (compAngleY + gyroYrate * dt) + 0.07 * pitch;

	  // Reset the gyro angle when it has drifted too much
	  if (gyroXangle < -180 || gyroXangle > 180)
	    gyroXangle = kalAngleX;
	  if (gyroYangle < -180 || gyroYangle > 180)
	    gyroYangle = kalAngleY;

	  /* Print Data */
	#if 0 // Set to 1 to activate
	  Serial.print(accX); Serial.print("\t");
	  Serial.print(accY); Serial.print("\t");
	  Serial.print(accZ); Serial.print("\t");

	  Serial.print(gyroX); Serial.print("\t");
	  Serial.print(gyroY); Serial.print("\t");
	  Serial.print(gyroZ); Serial.print("\t");

	  Serial.print("\t");
	#endif

	  Serial.print("roll: "); Serial.print(roll); Serial.print("\n");
	  Serial.print("gyroXangle: ");Serial.print(gyroXangle); Serial.print("\n");
	  Serial.print("compAngleX: ");Serial.print(compAngleX); Serial.print("\n");
	  Serial.print("kalAngleX: ");Serial.print(kalAngleX); Serial.print("\n");

	  Serial.print("\n");

	  Serial.print("pitch: ");Serial.print(pitch); Serial.print("\n");
	  Serial.print("gyroYangle: ");Serial.print(gyroYangle); Serial.print("\n");
	  Serial.print("compAngleY: ");Serial.print(compAngleY); Serial.print("\n");
	  Serial.print("kalAngleY: ");Serial.print(kalAngleY); Serial.print("\n");

	#if 0 // Set to 1 to print the temperature
	  Serial.print("\t");

	  double temperature = (double)tempRaw / 340.0 + 36.53;
	  Serial.print(temperature); Serial.print("\t");
	#endif

	  Serial.print("\r\n");
	  delay(2);
	  delay(333);
}

// old code below:

// #include <math.h>

// #define GYR_Y 0  // Gyro Y (IMU pin #4)
// #define ACC_Z 1  // Acc  Z (IMU pin #7)
// #define ACC_X 2  // Acc  X (IMU pin #9)

// int sensorValue[3] = {0, 0, 0};
// int sensorZero[3] = {0, 0, 0};

// int LOOP_DURATION = 9; // STD_LOOP_TIME how long each loop is
// int lastLoopTime = LOOP_DURATION; // 
// int currentLoopTime = LOOP_DURATION; // lastLoopUsefulTime how many seconds have passed since loop started
// unsigned long loopStartTime = 0;


// void setup() {
//  // analogReference(EXTERNAL); // Aref 3.3V
//  Serial.begin(115200);
//  delay(100);
//  // calibrateSensors();
// }

// void loop() {

//  // loop timing control
//  currentLoopTime = millis() - loopStartTime; 
//  if (currentLoopTime < LOOP_DURATION) { // change use of delay to a time ticker when asynchronous processes needed later
//    delay(LOOP_DURATION - currentLoopTime);
//  }
//  lastLoopTime = millis() - loopStartTime;
//  loopStartTime = millis();

//  serialOut_timing();
// }

// void serialOut_timing() {

//  static int skip = 0;
//  if (skip++ == 50) { // display every 500 ms (at 100 Hz)
//    skip = 0;
//    Serial.print(currentLoopTime); Serial.print(",");
//    Serial.print(lastLoopTime); Serial.print("\n");
//    Serial.print(loopStartTime); Serial.print("\n");
//    Serial.print(skip); Serial.print(" le\n");
//  }
//  // Serial.print(skip); Serial.print(" he\n");
// }

// // Set zero sensor values
// void calibrateSensors() {      

//  long v;
//  for(int n=0; n<3; n++) {
//    v = 0;
//    for(int i=0; i<50; i++) {
//      v += readSensor(n);
//    }
//    sensorZero[n] = v/50;
//  }
//  sensorZero[ACC_Z] -= 102;
// }

// int readSensor(int channel) {
//  return (analogRead(channel));
// }
