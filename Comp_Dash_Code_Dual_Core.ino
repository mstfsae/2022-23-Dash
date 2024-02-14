#include <CAN.h>
#include <CAN_config.h>
#include <ESP32CAN.h>
#include <can_regdef.h>
#include <TFT_eSPI.h>
#include <SPI.H>
#include "button.h"
TFT_eSPI tft = TFT_eSPI();
CAN_device_t CAN_cfg;
const int rx_queue_size = 10;       // Receive Queue size
//rev light, gear ratio, engine temp, fuel&oil level/pressure

// library for CAN https://github.com/miwagner/ESP32-Arduino-CAN

//Defining pins on the circuit board to which hole number--------------------
#define Shift_all 16
#define Shift_five 4
#define Shift_four 0
#define Shift_three 2
#define Shift_two 15
#define Shift_one 13

// **** LEAVE COMMENTED OUT *****
// REFER TO SETUP FILE TO CONFIGURE PINS
// #define MOSI 23
// #define MISO 19
// #define SPICLCK 18
// #define reset 5
// #define chip_select 17  //for screen
// #define dc 21
// #define tft_led 14
// #define tft_tcs 12
// #define tft_irq 22




// ***** Defining switches *****
#define switchOne 27
#define switchTwo 26
#define switchThree 25
#define switchFour 33 // CHECK THIS
#define switchFive 32 // CHECK THIS


// ***** DEFINING CONSTANTS *****
#define DEG2RAD 0.0174532925


#define COOLANT_WARNING_TEMP 112
//#define COOLANT_WARNING_TEMP 50


//ESP32 dual core
TaskHandle_t Task1;
TaskHandle_t Task2;



Button switch1;
Button switch2;
Button switch3;
Button switch4;
Button switch5;

//********************************Global Initialization***********************************
int dutyCycle = 200; // controls LED brightness
//  Used for stage6 blink without delay
bool instage6 = true;
unsigned long previousMillis = 0;
const long interval = 200; // RPM BLINK INTERVAL (in millis)
int ledState = LOW;
int oldThrottleBar;
int previousGear = 100; // set to 100 to start to ensure first gear printed is correct gear
int angle= 0; // for testing
int previousAngle = 1000; // set to 1000 at start to ensure starting position
uint16_t fuelPressure = 0;// change to 0 after testing
uint8_t engineTemp = 0;
uint16_t engineoilPres = 0;
uint16_t previousFuelPressure = 0;
uint8_t previousEngineTemp = 0;
uint16_t previousengineoilPressure = 0;
uint16_t previousEngineRPM = 0;
uint16_t throttlePos = 0;
uint8_t battvolt = 0;
uint16_t enrpm = 0;

char gear;

bool shiftLightsActive = false;




uint16_t FlipBytes_2B_Temp1 = 0;
uint16_t FlipBytes_2B_Temp2 = 0;



int rpm = 0;// change to 0 for use




//Functions----------------
//six functions for led lights bb,gg,rr,gg,bb
//1. stages for shift lights
void stage1() {
  analogWrite(Shift_one, dutyCycle);
  analogWrite(Shift_two, 0);
  analogWrite(Shift_three, 0);
  analogWrite(Shift_four, 0);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  shiftLightsActive = true;
}


void stage2() {
  analogWrite(Shift_one, dutyCycle);
  analogWrite(Shift_two, dutyCycle);
  analogWrite(Shift_three, 0);
  analogWrite(Shift_four, 0);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  shiftLightsActive = true;
}


void stage3() {
  analogWrite(Shift_one, dutyCycle);
  analogWrite(Shift_two, dutyCycle);
  analogWrite(Shift_three, dutyCycle);
  analogWrite(Shift_four, 0);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  shiftLightsActive = true;
}


void stage4() {
  analogWrite(Shift_one, dutyCycle);
  analogWrite(Shift_two, dutyCycle);
  analogWrite(Shift_three, dutyCycle);
  analogWrite(Shift_four, dutyCycle);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  shiftLightsActive = true;
}

void stage5() {
  analogWrite(Shift_one, dutyCycle);
  analogWrite(Shift_two, dutyCycle);
  analogWrite(Shift_three, dutyCycle);
  analogWrite(Shift_four, dutyCycle);
  analogWrite(Shift_five, dutyCycle);
  analogWrite(Shift_all, 0);

  shiftLightsActive = true;
}

void stage6() {
  // executes while in stage 6
  int currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval){
    previousMillis = millis();
    if(ledState == LOW){
     analogWrite(Shift_one, 0);
     analogWrite(Shift_two, 0);
     analogWrite(Shift_three, 0);
     analogWrite(Shift_four, 0);
     analogWrite(Shift_five, dutyCycle);
     analogWrite(Shift_all, dutyCycle);
     ledState = HIGH;
    }
     else {
     analogWrite(Shift_one, 0);
     analogWrite(Shift_two, 0);
     analogWrite(Shift_three, 0);
     analogWrite(Shift_four, 0);
     analogWrite(Shift_five, 0);
     analogWrite(Shift_all, 0);
     ledState = LOW;
     }
  }

  shiftLightsActive = true;
 
}

//all lights off
void stage0() {
  analogWrite(Shift_one, 0);
  analogWrite(Shift_two, 0);
  analogWrite(Shift_three, 0);
  analogWrite(Shift_four, 0);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  shiftLightsActive = false;
}

// Controls Shift light brightness by adjusting dutyCycle. Increments in chunks of 51.... 5 settings                  
void ledBrightness(){
  if(dutyCycle < 255){
    dutyCycle+= 51;
  }
  else{
    dutyCycle = 0;
  }
}


// 2. Functions that does it all
void set_light_stage(uint16_t current_rpm, char current_gear) {
  current_rpm = (FlipBytes_2B(current_rpm));
  current_gear = current_gear & 0x0F;
  
  if (current_gear == 0b00001111) {
      if ((current_rpm >= 11000) && (current_rpm < 11400)) {
        stage1();
      } else if ((current_rpm >= 11400) && (current_rpm < 11800)) {
        stage2();
      } else if ((current_rpm >= 11800) && (current_rpm < 12200)) {
        stage3();
      } else if ((current_rpm >= 12200) && (current_rpm < 12600)) {
        stage4();
      } else if ((current_rpm >= 12600) && (current_rpm < 13000)) {
        stage5();
      } else if ((current_rpm >= 13000)) {
        stage6();
      } else {
        stage0();
      }
  }
    else if (current_gear == 0b00000001){

      if ((current_rpm >= 11000) && (current_rpm < 11400)) {
        stage1();
      } else if ((current_rpm >= 11400) && (current_rpm < 11800)) {
        stage2();
      } else if ((current_rpm >= 11800) && (current_rpm < 12200)) {
        stage3();
      } else if ((current_rpm >= 12200) && (current_rpm < 12600)) {
        stage4();
      } else if ((current_rpm >= 12600) && (current_rpm < 13000)) {
        stage5();
      } else if ((current_rpm >= 13000)) {
        stage6();
      } else {
        stage0();
      }
    }


    else if (current_gear == 0b00000010){

      if ((current_rpm > 10200) && (current_rpm < 10600)) {
        stage1();
      } else if ((current_rpm >= 10600) && (current_rpm < 11000)) {
        stage2();
      } else if ((current_rpm >= 11000) && (current_rpm < 11400)) {
        stage3();
      } else if ((current_rpm >= 11400) && (current_rpm < 11800)) {
        stage4();
      } else if ((current_rpm >= 11800) && (current_rpm < 12200)) {
        stage5();
      } else if (current_rpm >= 12200) {
        stage6();
      } else {
        stage0();
      }
    }

      else if (current_gear == 0b00000011){

      if ((current_rpm > 12000) && (current_rpm < 12500)) {
        stage1();
      } else if ((current_rpm >= 12500) && (current_rpm < 13000)) {
        stage2();
      } else if ((current_rpm >= 13000) && (current_rpm < 13500)) {
        stage3();
      } else if ((current_rpm >= 13500) && (current_rpm < 14000)) {
        stage4();
      } else if ((current_rpm >= 14000) && (current_rpm < 14500)) {
        stage5();
      } else if (current_rpm >= 14500) {
        stage6();
      } else {
        stage0();
      }
      }
  }


void displayThrottleBar(uint16_t throttleVal)
{
  //Perform all calculations within display function
  throttleVal = FlipBytes_2B(throttleVal) / 10;

  //Serial print raw value to check what it is
  if (throttleVal >100){
    throttleVal = 100;
  }
  int HEIGHT = map(throttleVal,0,100,0,150);
  tft.fillRect(270,40,40, 150-HEIGHT, TFT_BLUE);
  tft.fillRect(270,190-HEIGHT,40, HEIGHT,TFT_GREEN); 
  tft.setCursor(270,205);
  tft.setTextSize(2);
  
  if (throttleVal < 10){
    tft.print("  " + String(throttleVal) + "%");
  }
  else if (throttleVal < 100){
    tft.print(" " + String(throttleVal) + "%");
  }
  else if( throttleVal < 1000){
    tft.print( String(throttleVal) + "%");
  }
  oldThrottleBar = HEIGHT;
}

void displayGearPosition(int gearPos){
  //Perform calculations within display function
  gearPos = gearPos & 0x0F;
  
  if((gearPos == previousGear)){
    return; 
  }
  switch(gearPos){
    case(0b00000000): // Neutral is 0
          if((previousGear != 0b00000010) && (previousGear != 0b00000011))
          {
            tft.fillRect(110,43,100,190,TFT_WHITE);
            tft.fillRect(110,43,20,161,TFT_BLACK);//legs of N
            tft.fillRect(190,43,20,161,TFT_BLACK);

            tft.drawLine(120-7,43,190-5,204,TFT_BLACK);
            tft.drawLine(120-6,43,190-5,204,TFT_BLACK);
            tft.drawLine(120-5,43,190-5,204,TFT_BLACK);
            tft.drawLine(120-4,43,190-4,204,TFT_BLACK);
            tft.drawLine(120-3,43,190-3,204,TFT_BLACK);
            tft.drawLine(120-2,43,190-2,204,TFT_BLACK);// draw extra lines to thicken lines cause fuck me i guess
            tft.drawLine(120-1,43,190-1,204,TFT_BLACK);
            tft.drawLine(120,43,190,204,TFT_BLACK);
            tft.drawLine(120+1,43,190+1,204,TFT_BLACK);
            tft.drawLine(120+2,43,190+2,204,TFT_BLACK);
            tft.drawLine(120+3,43,190+3,204,TFT_BLACK);   
            tft.drawLine(120+4,43,190+4,204,TFT_BLACK);
            tft.drawLine(120+5,43,190+5,204,TFT_BLACK);
            tft.drawLine(120+6,43,190+5,204,TFT_BLACK);
            tft.drawLine(120+7,43,190+7,204,TFT_BLACK);
          }
          
      break;
    case(0b00001111):
        tft.fillRect(110,43,100,190,TFT_WHITE);

         tft.fillRect(150,43,20,161,TFT_BLACK);
         tft.fillRect(110,197,100,20,TFT_BLACK);

         tft.fillRect(130,63,20,20,TFT_BLACK);
         
         break;
    case(0b00000001):
         tft.fillRect(110,43,100,190,TFT_WHITE);
         tft.fillRect(110,43,100,20,TFT_BLACK);
         tft.fillRect(190,43,20,77,TFT_BLACK);
         tft.fillRect(110,120,100,20,TFT_BLACK);
         tft.fillRect(110,120,20,77,TFT_BLACK);
         tft.fillRect(110,197,100,20,TFT_BLACK);
         break;
    case(0b00000010):
         tft.fillRect(110,43,100,190,TFT_WHITE);
         tft.fillRect(110,43,100,20,TFT_BLACK);
         tft.fillRect(190,43,20,174,TFT_BLACK);
         tft.fillRect(110,120,100,20,TFT_BLACK);
         tft.fillRect(110,197,100,20,TFT_BLACK);
         break;      
    case(0b00000011):
         tft.fillRect(110,43,100,190,TFT_WHITE);
         tft.fillRect(190,43,20,174,TFT_BLACK);
         tft.fillRect(110,120,100,20,TFT_BLACK);
         tft.fillRect(110,43,20,77,TFT_BLACK);
         break;      
     
  }
   previousGear = gearPos;

   return;
}

//void displaySteeringAngle(int angle){ // -125 to 125
//    if(angle == previousAngle){
//    return; 
//  }
//  else {
//    tft.fillRect(90,20,140,10,TFT_GREEN);
//    
//  }
//    int angleMapped = map(angle,-125,125,-70,70);
//    previousAngle = angle;
//    if(angleMapped > 0){
//
//      tft.fillRect(155,23,angleMapped,4,TFT_RED);
//    }
//    if(angleMapped < 0){
//      tft.fillRect(155 + angleMapped,23,abs(angleMapped),4,TFT_RED);
//
//    }
//
//    tft.fillRect(155,18,3,14,TFT_WHITE);
//  }


void displayFuelPressure(uint16_t fuelPres){

  //ECU is kpa, dash should be psi. psi = kpa / 6.895
  fuelPres = (FlipBytes_2B(fuelPres)) / 6.895; //This value is the fuel pressure multiplied by 10
  
  if((previousFuelPressure == fuelPres) && (previousFuelPressure != 0))
  {
    return;
  }
        
        // if(previousFuelPressure == fuelPres){
        //   return;
        // }

        //(x,y,width,height,color)
  tft.fillRect(5,5,70,55,TFT_RED);
  tft.setCursor(20,10);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_RED);

  tft.print("  Fuel");
  tft.setCursor(20,20);
  tft.print("Pressure");
  tft.setCursor(20,40);
    
  if ((fuelPres / 10) < 10){
  tft.print("  " + String(fuelPres / 10) + "." + (fuelPres % 10) + " psi");
  }
  else if ((fuelPres / 10) < 100){
    tft.print(" " + String(fuelPres / 10) + "." + (fuelPres % 10) + " psi");
  }
  else if( (fuelPres / 10) < 1000){
    tft.print(String(fuelPres / 10) + "." + (fuelPres % 10) + " psi");
  }
  previousFuelPressure = fuelPres;
  return;
}
//(x,y,width,height,color)
// void displayEngineTemp(int engineTemp){
//   // if(previousEngineTemp == engineTemp)
//   // {
//   //   return;
//   // }
//   tft.fillRect(5,60,75,45,TFT_GREEN);
//   tft.setTextSize(1);
//   tft.setTextColor(TFT_WHITE, TFT_GREEN);

//   tft.setCursor(10,70);
//   tft.print(" Engine");
//   tft.setCursor(10,80);
//   tft.print("  Temp");

//   //reset cursor
//   tft.setCursor(25,95);

  

//   if(engineTemp < 10){
//     tft.print(" " + String(engineTemp));
//   }
//     else if(engineTemp < 100 ){
//       tft.print(" " + String(engineTemp));
//     }
//     else if(engineTemp < 1000){
//       tft.print(String(engineTemp));
//     }
//       previousEngineTemp = engineTemp;
//       return;
// } 


void displayEngineOilPresh(uint16_t engineoilPres){

  engineoilPres = (FlipBytes_2B(engineoilPres)) / 6.895;
  
  if((previousengineoilPressure == engineoilPres) && (previousengineoilPressure != 0))
  {
    return;
  }


  // if(previousengineoilPressure == engineoilPres)
  // {
  //   return;
  // }
  tft.fillRect(10,60,70,55,TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);

  tft.setCursor(10,75);
  tft.print("Engine Oil");
  tft.setCursor(10,85);
  tft.print("Pressure");

  //reset cursor
  tft.setCursor(10,95);

  if ((engineoilPres / 10) < 10){
  tft.print("  " + String(engineoilPres / 10) + " psi");
  }
  else if ((engineoilPres / 10) < 100){
    tft.print(" " + String(engineoilPres / 10) +  " psi");
  }
  else if( (engineoilPres / 10) < 1000){
    tft.print(String(engineoilPres / 10) + " psi");
  }
  previousengineoilPressure = engineoilPres;
  return;
}

//   if(oilTemp < 10){
//     tft.print(" " + String(oilTemp));
//   }
//     else if(oilTemp < 100 ){
//       tft.print(" " + String(oilTemp));
//     }
//     else if(oilTemp < 1000){
//       tft.print(String(oilTemp));
//     }
//       previousOilTemp = oilTemp;
//       return;
// } 


//(x,y,width,height,color)
void displayEngTemp(uint8_t engineTemp){

  engineTemp = engineTemp - 40;
  
  if((previousEngineTemp == engineTemp) && (previousEngineTemp != 0))
  {
    return;
  }


  tft.setCursor(15,100);
  tft.fillRect(10,150,60,55,TFT_GREEN);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);

  tft.setCursor(10,160);
  tft.print("Engine");
  tft.setCursor(15,170);
  tft.print("Temp");

  //reset cursor
  tft.setCursor(10,180);
  if (engineTemp < 10){
  tft.print("  " + String(engineTemp) + " C");
  }
  else if (engineTemp < 100){
    tft.print(" " + String(engineTemp) +  " C");
  }
  else if(engineTemp < 1000){
    tft.print(String(engineTemp) + " C");
  }
  previousEngineTemp = engineTemp;

  if (engineTemp >= COOLANT_WARNING_TEMP)
  {
    if(shiftLightsActive)
    {
      tft.fillRect(90,0,140,100,TFT_RED);
      tft.setTextSize(2);
      tft.setCursor(100, 10);
      tft.print("Coolant Overtemp!");
    }
    else
    {
      analogWrite(Shift_three, dutyCycle);
    }
  }
  else if (engineTemp < COOLANT_WARNING_TEMP)
  {
    if(shiftLightsActive)
    {
      tft.fillRect(90,0,140,100,TFT_WHITE);
    }
    else
    {
      stage0();
    }
  }

  return;
}


void displayEngineRPM(uint16_t enrpm){

  enrpm = (FlipBytes_2B(enrpm)); // should zero all bits in 64 bit number except bits 0 to 16
  
  if((previousEngineRPM == enrpm) && (previousEngineRPM != 0))
  {
    return;
  }


  tft.fillRect(10,110,60,45,TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  tft.setCursor(10,130);
  tft.print("Engine RPM");

  //reset cursor
  tft.setCursor(5,140);

  if (enrpm < 10){
  tft.print("    " + String(enrpm) + " rpm");
  }
  else if (enrpm < 100){
    tft.print("   " + String(enrpm) +  " rpm");
  }
  else if(enrpm < 1000){
    tft.print("  " + String(enrpm) + " rpm");
  }
  else if (enrpm < 10000){
    tft.print(" " + String(enrpm) +  " rpm");
  }
  else if(enrpm < 100000){
    tft.print(String(enrpm) + " rpm");
  }
  previousEngineRPM = enrpm;
  return;
          
} 

void displayBatteryVoltage(uint8_t battvolt){
  tft.setTextColor(TFT_BLUE, TFT_WHITE);
  tft.setCursor(3,210);
  tft.setTextSize(2);
  if((battvolt / 10) < 10)
  {
    tft.print(" ");
  }
  tft.print(String(battvolt / 10) + "." + (battvolt % 10) + "V");
  return;
}
void receiveCAN(){

  //NOTE: CAN sends data 8 bits at a time, so they are not all in the same order you would expect
  //For example (rx_frame.data.u64 >> 48) does not get you the correct 16 bit value with 48 bit offset
  //The format for this would then be LLLLLLLLHHHHHHHH, so you need to swap bytes
    
    CAN_frame_t rx_frame;



  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    // if (rx_frame.FIR.B.FF == CAN_frame_std) {
    //   // printf("New standard frame");
    // }
    // else {
    //   // printf("New extended frame");
    // }

    if (rx_frame.FIR.B.RTR == CAN_RTR) {
      // printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    }
    else {
      // printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
      for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
        // printf("0x%02X ", rx_frame.data.u8[i]);
      }
      if(rx_frame.MsgID == 0x640){ // 0x640 == (speed[0:16], MAP[16:32], MAPT[32:48], ThrottlePos[48:64])
        enrpm = (rx_frame.data.u64); // should zero all bits in 64 bit number except bits 0 to 16
        throttlePos = (rx_frame.data.u64 >> 48);
      }
      if(rx_frame.MsgID == 0x641){ // 0x641 = (fuelvolume[0:16], fuelMixture[16:32],fuelPressure[32:48],rest is useless)
        //ECU is kpa, dash should be psi. psi = kpa / 6.895
        fuelPressure = (rx_frame.data.u64 >> 32); //This value is the fuel pressure multiplied by 10
      }
      if(rx_frame.MsgID == 0x64D){ //gear ratio
        gear = (rx_frame.data.u64 >> 48);
      }
      if(rx_frame.MsgID == 0x649){ //battery voltage: 
        battvolt =  (rx_frame.data.u64 >> 40);
        engineTemp = (rx_frame.data.u64);
      }
      if(rx_frame.MsgID == 0x644){
        engineoilPres = (rx_frame.data.u64 >> 48);
      }
      
      
      //printf("\n");
    }
  }
}

void setup() {

//  Serial.begin(115200);
  

  //pin modes, input
  pinMode(switchOne, INPUT);
  pinMode(switchTwo, INPUT);
  pinMode(switchThree, INPUT);
  pinMode(switchFour, INPUT);
  pinMode(switchFive, INPUT);

  //pin modes, output
  pinMode(Shift_all, OUTPUT);
  pinMode(Shift_five, OUTPUT);
  pinMode(Shift_four, OUTPUT);
  pinMode(Shift_three, OUTPUT);
  pinMode(Shift_two, OUTPUT);
  pinMode(Shift_one, OUTPUT);
//  pinMode(tft_led, OUTPUT);
 // pinMode(chip_select, OUTPUT);
  // pinMode(tft_tcs, OUTPUT);
  // pinMode(reset, OUTPUT);
  // pinMode(dc, OUTPUT);


  //defaulting the shiftlights to 0/0/off
  analogWrite(Shift_one, 0);
  analogWrite(Shift_two, 0);
  analogWrite(Shift_three, 0);
  analogWrite(Shift_four, 0);
  analogWrite(Shift_five, 0);
  analogWrite(Shift_all, 0);

  // ********************* TFT SETUP *****************

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setTextSize(2);
  



// DISPLAY OBJECT SETUP
  tft.fillRect(260,30,60,170,TFT_BLUE); // blue background for Throttle Position
  tft.fillRect(260,180,20,2,TFT_RED);// throttle pos line
  tft.fillRect(310,180,20,2,TFT_RED);// throttle pos line
  // Prints Steering Angle Label
  tft.setCursor(115,8); 
  tft.setTextSize(1);
  // tft.print("Steering Angle");



  // SWITCH SETUP
  switch1.begin(switchOne);
  switch2.begin(switchTwo);
  switch3.begin(switchThree);
  switch4.begin(switchFour);
  switch5.begin(switchFive);

  // CAN Setup
  CAN_cfg.speed = CAN_SPEED_1000KBPS; // tune shows CAN 1 at 1Mbps
  CAN_cfg.tx_pin_id = GPIO_NUM_25;
  CAN_cfg.rx_pin_id = GPIO_NUM_35;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();




  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Core1Code,   /* Task function. */
                    "Core1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Core2Code,   /* Task function. */
                    "Core2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500);
  
  }


//Code running on core 1
void Core1Code( void * pvParameters )
{
  // Serial.print("Task1 running on core ");
  // Serial.println(xPortGetCoreID());

  for(;;){
    receiveCAN();
  } 
}

//Code running on core 2
void Core2Code( void * pvParameters )
{
  // Serial.print("Task2 running on core ");
  // Serial.println(xPortGetCoreID());

  for(;;){
    displayFuelPressure(fuelPressure);
    displayBatteryVoltage(battvolt);
    displayEngineOilPresh(engineoilPres);
    displayEngineRPM(enrpm);
    displayEngTemp(engineTemp);
    displayThrottleBar(throttlePos);
    displayGearPosition(gear);

    set_light_stage(enrpm, gear);
  }
}



void loop() {
  
    // if(switch1.debounce()){ // checks if SW1 pressed ***** CHANGES SHIFT LIGHT BRIGHTNESS *****
    //   ledBrightness();
    // }
//    if(switch5.debounce()){ 
//      angle+=1;
//    }
//    if(switch4.debounce()){
//      angle-=1;
//    }
//    if(switch3.debounce()){
//      fuelPressure+=1;
//    }
//    if(switch2.debounce()){
//      fuelPressure-=1;
//    }

    // receiveCAN();
        

        // tft.setTextSize(1);
        // Serial.println(throttlePos);
        //printBits(throttlePos);

        // printBits16(fuelPressure);
    
         
        //  tft.setTextSize(2);       
        //  tft.setTextColor(TFT_BLACK,TFT_WHITE);
        //  set_light_stage(rpm, gear);

        // displayThrottleBar(throttlePos);
        // displayGearPosition(gear);
        // displaySteeringAngle(angle);

        // displayFuelPressure(fuelPressure);
        // displayBatteryVoltage(battvolt);
        // displayEngineOilPresh(engineoilPres);
        // displayEngineRPM(enrpm);
        // displayEngTemp(engineTemp);
        // tft.setCursor(160,120);
        //tft.print(wheelFL);
        
        //tft.fillScreen(TFT_BLACK);

    }
    
    void printBits16(int b)
    {
      for(int i = 15; i >= 0; i--)
      {
        Serial.print(bitRead(b,i));
        if(i%4 == 0) Serial.print(" ");
      }
      Serial.println();
    }
    
    void printBits8(char b)
    {
      for(int i = 7; i >= 0; i--)
      {
        Serial.print(bitRead(b,i));
        if(i%4 == 0) Serial.print(" ");
      }
      Serial.println();
    }

    uint16_t FlipBytes_2B(uint16_t input)
    {
      FlipBytes_2B_Temp1 = input >> 8;
      FlipBytes_2B_Temp2 = ((uint8_t)input) << 8;
      
      return FlipBytes_2B_Temp1 + FlipBytes_2B_Temp2;
    }
  
