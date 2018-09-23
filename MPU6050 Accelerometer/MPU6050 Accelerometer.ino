
#include <EEPROM.h>
#include <Wire.h>

const byte led = 13;
const byte gsmStart = 3;
const byte alarmPin = 5;
const byte rfPin = 9;
const byte piezo = 4;

String phoneNumber = "";
String val = "";

boolean ARM = true;
boolean alarmState = false;

boolean answer = false;
boolean rfState = 0;
boolean lastRfState = 0;

unsigned long interval = 6000;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned long set = 0;
unsigned long interval1 = 10000;



float varVolt = 61.3;  // среднее отклонение (ищем в excel)
float varProcess = 0.01; // скорость реакции на изменение (подбирается вручную)
float Pc = 0.0;
float G = 0.0;
float P = 1.0;
float Xp = 0.0;
float Zp = 0.0;
float Xe = 0.0;
float fixPosX = 0.0;
float fixPosY = 0.0;
float fixPosZ = 0.0;

unsigned long currentMillis = 0;

boolean fixPoseStatus = 0;
boolean alarmStatus = 0;
int i = 0;

long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;

long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;




void setup() {
  Wire.begin();
  Serial.begin(9600);
  setupMPU();
}


void loop() {
  recordAccelRegisters();
  recordGyroRegisters();
  printData();
}


void setupMPU() {      // აქსელერომეტრის კონფიგურაცია ძილიდან გამოყვანა და ასშ
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s
  Wire.endTransmission();
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
  Wire.write(0b00000000); //Setting the accel to +/- 2g
  Wire.endTransmission();
}

void recordAccelRegisters() {      // აქსელერომეტრის მონაცემების რეგისტრებიდან გამოტანა
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000, 6); //Request Accel Registers (3B - 40)
  while (Wire.available() < 6);
  accelX = Wire.read() << 8 | Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read() << 8 | Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read() << 8 | Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void processAccelData() {       // აქსელერომეტრის მონაცემების დამუშავება ჯი ერთეულში
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0;
  gForceZ = accelZ / 16384.0;
}

void recordGyroRegisters() {     // გიროსკოპის მონაცემების ამოღება რეგისტრებიდან
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000, 6); //Request Gyro Registers (43 - 48)
  while (Wire.available() < 6);
  gyroX = Wire.read() << 8 | Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read() << 8 | Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read() << 8 | Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {    // გიროსკოპის ინფორმაციის კონვერტაცია სი სისტემაში
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0;
  rotZ = gyroZ / 131.0;
}

void printData() {    // აქსელერომეტრის მონაცემების გამოტანა სერიალზე
  Serial.print(filter(rotX));
  Serial.print(",");
  Serial.print(filter(rotY));
  Serial.print(",");
  Serial.print(filter(rotZ));
  Serial.print(",");
  Serial.println("");
}

float filter(float val) {  // კალმანის ფილტრი
  Pc = P + varProcess;
  G = Pc / (Pc + varVolt);
  P = (1 - G) * Pc;
  Xp = Xe;
  Zp = Xp;
  Xe = G * (val - Zp) + Xp; // "фильтрованное" значение
  return (Xe);
}
