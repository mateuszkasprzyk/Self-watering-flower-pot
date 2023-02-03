/*
  ReadAnalogVoltage

  Reads an analog input on pin 0, converts it to voltage, and prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/ReadAnalogVoltage
*/

#include <LiquidCrystal.h>
#include <Timers.h>
#include <EEPROM.h>


const int rs = 7, en = 6, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//=============general pins============
const int soilSensorPower = 14;
const int soilSensorData = A5;
const int pumpPower = 9;
const int waterLevelSensorPin = 12;
const int buzzerPin = 8;

//general variables=======================
float soilHumidity = 100;
bool isWaterIntank = false;

int performReset=false;
unsigned long requestedHumidity;
//==========encoder====================
const int data_pin = 16, clk_pin = 15, encoderSwitchPin = 17;
bool wasEncoderPressed = false;
bool wasEncoderSwitchReleased = false;
int counter=0,new_position=0,old_position=1;
unsigned long encoderSwitchTime = 3000;
Timer encoderTimer;
//=========timers=====================
Timer humidityTimer,screenUpdateTimer,wateringTimer,screenModeTimer;
int screenUpdatePeriod = 300;
int screenModePeriod = 5000;
int screenMode = 0;
unsigned long wateringPeriod = 0;
unsigned long humiditySamplePeriod = 0;
unsigned long pumpingTime = 0;

unsigned long encoderPressedTime = 0;
unsigned long encoderReleasedTime = 0;
unsigned long encoderClickTime = 0;


void DisplayMenu(int option){
  if(screenUpdateTimer.available()){
    lcd.clear();
    lcd.setCursor(0, 0);
    switch(option){
        case 0:
          lcd.print("Humidity time: ");
          lcd.setCursor(0, 1);
          lcd.print(humiditySamplePeriod);
          lcd.print(" s");
        break;
        case 1:
          lcd.print("Pumping time: ");
          lcd.setCursor(0, 1);
          lcd.print(pumpingTime);
          lcd.print(" ms");
        break;
        case 2:
          lcd.print("Watering time: ");
          lcd.setCursor(0, 1);
          lcd.print(wateringPeriod);
          lcd.print(" s");
        break;
        case 3:
          lcd.print("Target hum.: ");
          lcd.setCursor(0, 1);
          lcd.print(requestedHumidity);
          lcd.print(" %");
        break;
        case 4:
          lcd.print("Default settings: ");
          lcd.setCursor(0, 1);
          lcd.print(performReset);
          Serial.println(performReset);
        break;
      }
    screenUpdateTimer.restart();   
  }
    
}

void optionsLoop(){
  performReset=0;
  int optionsIndex = 0;
  bool wasEncoderSwitchReleased = false;
  bool stayInMenu = true;
  
  while(stayInMenu){
      DisplayMenu(optionsIndex);
      switch(optionsIndex){
        case 0:
         humiditySamplePeriod = encoderPosition(humiditySamplePeriod,1);
        break;
        case 1:
         pumpingTime = encoderPosition(pumpingTime,10);
        break;
        case 2:
         wateringPeriod = encoderPosition(wateringPeriod,1);
        break;
        case 3:
         requestedHumidity = encoderPosition(requestedHumidity,1);
        break;
        case 4:
         performReset = encoderPosition(performReset,1);
         if(performReset>1)
          performReset = 0;
         if(performReset<0)
          performReset = 1;
        break;
        }
      
      checkEncoderClick(0);

      if(!wasEncoderSwitchReleased){
          wasEncoderSwitchReleased = wasEncoderReleased();
        }
        
      if(wasEncoderSwitchReleased){
          if(checkEncoderClick(3000)){
          stayInMenu = false;  
          }
          if(clickCheck()){
              Serial.println("click");
              optionsIndex++;
              if(optionsIndex>4)
                optionsIndex=0;
           }
      }
    }

}
bool checkEncoderClick(int timeInMili){
  if(!wasEncoderPressed){
    if(checkEncoderSwitch()){
      wasEncoderPressed = true;
      encoderPressedTime = millis();
    }
  }else{
      if(checkEncoderSwitch()){
          if(millis() - encoderPressedTime > timeInMili){
            return true;  
          }
      }else{
          wasEncoderPressed = false;
          encoderReleasedTime = millis(); 
      }
  }
  return false;
}

bool clickCheck(){
  if(millis() - encoderClickTime > 1000 && millis()-encoderReleasedTime < 1000 && encoderReleasedTime - encoderPressedTime<300){
      encoderClickTime = millis();
      return true;
    }
    return false;  
}
bool wasEncoderReleased(){
  if(!wasEncoderPressed){
    return true;  
  }
  return false;
}

void updateDisplay(){
  if(screenModeTimer.available()){
    screenMode++;
    if(screenMode>3)
      screenMode = 0;
    screenModeTimer.restart(); 
  }

  if(screenUpdateTimer.available()){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hum: ");
    lcd.print(String (soilHumidity));
    lcd.print("%");
    
    switch(screenMode){
      case 0:
        lcd.setCursor(0, 1);
        lcd.print("Next wat.");
        lcd.print(wateringTimer.time()/1000);
        lcd.print(" s");
      break;
      case 1:
        lcd.setCursor(0, 1);
        lcd.print("Target hum.: ");
        lcd.print(requestedHumidity);
        lcd.print("%");
      break;
      case 2:
        lcd.setCursor(0, 1);
        lcd.print("Hum check: ");
        lcd.print(humidityTimer.time()/1000);
        lcd.print(" s");
      break;
      case 3:
        lcd.setCursor(0, 1);
        lcd.print("Pump t.: ");
        lcd.print(pumpingTime);
        lcd.print(" ms");
      break;
    }

    screenUpdateTimer.restart(); 
  }
}

int encoderPosition(int input,int interval){
  
  new_position = digitalRead(clk_pin);

  if (new_position != old_position &&new_position==1){     
     if (digitalRead(data_pin) != new_position) { 
       input -=interval;
     }
     else {
       input+=interval;
     }
  }   
  old_position = new_position;
  return input; 
}
void defaultSettings(){
  humiditySamplePeriod = 1;
  pumpingTime = 500;
  wateringPeriod = 30;
  requestedHumidity = 50;
}
void saveSettings(){
  int add=0;
  EEPROM.put(add,humiditySamplePeriod);
  add+=sizeof(unsigned long);
  EEPROM.put(add,pumpingTime);
  add+=sizeof(unsigned long);
  EEPROM.put(add,wateringPeriod);
  add+=sizeof(unsigned long);
  EEPROM.put(add,requestedHumidity);
}
void loadSettings(){
  int add=0;
  humiditySamplePeriod = EEPROM.get(add,humiditySamplePeriod);
  add+=sizeof(unsigned long);
  pumpingTime = EEPROM.get(add,pumpingTime);
  add+=sizeof(unsigned long);
  wateringPeriod = EEPROM.get(add,wateringPeriod);
  add+=sizeof(unsigned long);
  requestedHumidity = EEPROM.get(add,requestedHumidity);
}
void playAlarm(){
  for(int i =0; i<3;i++){
      tone(buzzerPin,100,250);
      delay(1000);
    }

}

void switchWaterPump(bool expected){
  expected?digitalWrite(pumpPower,LOW):digitalWrite(pumpPower,HIGH);
}
  
float getSoilHumidity(){
  float result = 0;
  digitalWrite(soilSensorPower,HIGH);
  delay(300);
  ;
  int sensorValue = analogRead(soilSensorData);

  digitalWrite(soilSensorPower,LOW);
  float sensorVoltage = sensorValue * (5.0 / 1023.0);
  Serial.print("voltage: ");
  Serial.println(sensorVoltage);

  result = -24*(sensorVoltage)+120;
  
  if(result > 100)
    result = 100;
  if(result < 0)
    result = 0;
  Serial.print("Humidity: ");  
  Serial.println(result);
  
  return result;
}

bool checkWaterLevel(){
  bool resutl = digitalRead(waterLevelSensorPin);
  return resutl;
}

bool checkEncoderSwitch(){
  bool resutl = digitalRead(encoderSwitchPin);
  return !resutl;
}

void setup() {
  loadSettings();

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(soilSensorPower,OUTPUT);
  pinMode(soilSensorData,INPUT);
  pinMode(waterLevelSensorPin,INPUT);
  pinMode(encoderSwitchPin,INPUT_PULLUP);
  pinMode(pumpPower,OUTPUT);
  pinMode(buzzerPin,OUTPUT);

  switchWaterPump(false);
  
  lcd.begin(16, 2);
  lcd.print("Self watering");
  lcd.setCursor(0, 1);
  lcd.print("flower pot");
  delay(1500);
  
  lcd.setCursor(0,0);
  lcd.print("Made by");
  lcd.setCursor(0, 1);
  lcd.print("Mateusz Kasprzyk");
  delay(1500);
  
  lcd.setCursor(0, 0);
  lcd.print("Software version");
  lcd.setCursor(0, 1);
  lcd.print("1.0");
  delay(2000);


  humidityTimer.begin(humiditySamplePeriod*1000);
  screenUpdateTimer.begin(screenUpdatePeriod);
  wateringTimer.begin(wateringPeriod*1000);
  encoderTimer.begin(encoderSwitchTime);
  screenModeTimer.begin(screenModePeriod);

}

// the loop routine runs over and over again forever:
void loop() {
Serial.println(wateringPeriod);

checkEncoderClick(0);

if(wasEncoderReleased()){
  wasEncoderSwitchReleased = true;
  //Serial.println("released");
}else{
    //Serial.println("not released");
}

if(checkEncoderClick(3000) && wasEncoderSwitchReleased){
  wasEncoderSwitchReleased = false;
  Serial.println("entering menu");
  optionsLoop();
  if(performReset){
    defaultSettings();
    }
  saveSettings();
  humidityTimer.begin(humiditySamplePeriod*1000);
  wateringTimer.begin(wateringPeriod*1000);
  
}

if(humidityTimer.available()){
  soilHumidity = getSoilHumidity();
  humidityTimer.restart();
}

updateDisplay();

if(wateringTimer.available()){
  isWaterIntank = checkWaterLevel();
  if(isWaterIntank){
    if(soilHumidity < requestedHumidity){
        switchWaterPump(true);
        delay(pumpingTime);
        switchWaterPump(false);
    }else{
        switchWaterPump(false); 
    }
    
  }else{
      playAlarm();
  }
  wateringTimer.restart();
}
}
