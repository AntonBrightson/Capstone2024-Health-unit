/* This code works with MAX30102 + 128x32 OLED i2c + and Arduino  UNO
 * It's displays the Average BPM on the screen, with an animation and a buzzer  sound
 * everytime a heart pulse is detected
 * This is my capstone code for the health hub
 * Not complete, still buggy
 * Entails a pulse oximeter displaying data on an OLED screen, a thermistor temp sensor circuit displaying data on an 16x2 I2C LCD screen
*/

#include <Adafruit_GFX.h>        //OLED  libraries
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>    //OLED library
#include <SPI.h>
#include <Wire.h>
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate  calculating algorithm
#include <LiquidCrystal_I2C.h>  //LCD library

// Create an instance of the MAX30105 class to interact with the sensor
MAX30105 particleSensor;

// Define the size of the rates array for averaging BPM; can be adjusted for smoother results
const byte RATE_SIZE  = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array  of heart rate readings for averaging
byte rateSpot = 0;   // Index for inserting the next heart rate reading into the array
long lastBeat = 0;   // Timestamp of the last detected beat, used to calculate BPM
float beatsPerMinute; // Calculated heart rate in beats per minute
int beatAvg;           // Average heart rate after processing multiple readings

//Temp sensor display
int ThermistorPin = 0;
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

#define i2c_Address 0x3c
#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display  height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino  reset pin)

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

void setup() {  

//LCD initialisation
 lcd.init(); // initialize the lcd
 lcd.backlight();
 lcd.clear();                 // clear display
 lcd.setCursor(0, 0);         // move cursor to   (0, 0)
 lcd.print("Hello");        // print message at (0, 0)
 lcd.setCursor(1, 1);         // move cursor to   (2, 1)
 lcd.print("Getting Started"); // print message at (2, 1)
 delay(2000);                 // display the above for two seconds
  
  
//display.begin(SSD1306_SWITCHCAPVCC,  0x3C); //Start the OLED display
 Serial.begin(9600);
 delay(250); //wait for OLED to power up
 display.begin(i2c_Address, true); // Address 0x3C default
 display.display();
 delay(2000);

//Initialize sensor
 particleSensor.begin(Wire, I2C_SPEED_FAST); //Use default  I2C port, 400kHz speed
 particleSensor.setup(); //Configure sensor with default  settings
 particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to  indicate sensor is running

}

void loop()
{
  //Pulse Oximeter sensor w/OLED disp
 long irValue = particleSensor.getIR();  // Read the infrared value from the sensor

  if (checkForBeat(irValue) == true) { // Check if a heart beat is detected
    long delta = millis() - lastBeat; // Calculate the time between the current and last beat
    lastBeat = millis(); // Update lastBeat to the current time
 
    beatsPerMinute = 60 / (delta / 1000.0); // Calculate BPM
 
    // Ensure BPM is within a reasonable range before updating the rates array
    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the rates array
      rateSpot %= RATE_SIZE; // Wrap the rateSpot index to keep it within the bounds of the rates array
 
      // Compute the average of stored heart rates to smooth out the BPM
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Display BPM and SpO2 on the OLED screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("BPM: " + String(beatsPerMinute, 1));    // Display BPM with one decimal place
  //display.println("SpO2: " + String(spo2, 1) + "%"); // Display SpO2 percentage
  display.display();

  // Check if the sensor reading suggests that no finger is placed on the sensor
  if (irValue < 50000)
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(" No finger?");
 
  display.display();
}

// //Thermistor temp sensor w/ LCD screen disp
//   Vo = analogRead(ThermistorPin);
//   R2 = R1 * (1023.0 / (float)Vo - 1.0);
//   R2 = R1 * (1023.0 / (float)Vo - 1.0);
//   logR2 = log(R2);
//   T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
//   T = T - 273.15;
//   //T = (T * 9.0)/ 5.0 + 32.0; 
  
//   lcd.clear();                  // clear display
//   lcd.setCursor(0, 0);          // move cursor to   (3, 0)
//   lcd.print("Temp = ");        // print message at (3, 0)
//   lcd.print(T);
//   lcd.print(" °C");            // print the degree Celsius symbol
//   delay(2000);         // display the above for two seconds
//   lcd.clear(); 



  //Serial.print("IR=");
  //Serial.print(irValue);
  //Serial.print(", BPM=");
  //Serial.print(beatsPerMinute);
  //Serial.print(", Avg BPM=");
  //Serial.print(beatAvg);
 
 



// void loop() {
// //Thermistor temp sensor w/ LCD screen disp
//   Vo = analogRead(ThermistorPin);
//   R2 = R1 * (1023.0 / (float)Vo - 1.0);
//   R2 = R1 * (1023.0 / (float)Vo - 1.0);
//   logR2 = log(R2);
//   T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
//   T = T - 273.15;
//   //T = (T * 9.0)/ 5.0 + 32.0; 
  

//   lcd.clear();                  // clear display
//   lcd.setCursor(0, 0);          // move cursor to   (3, 0)
//   lcd.print("Temp = ");        // print message at (3, 0)
//   lcd.print(T);
//   lcd.print(" °C");            // print the degree Celsius symbol
//   delay(2000);         // display the above for two seconds
//   lcd.clear(); 

//  long irValue = particleSensor.getIR();  // Read the infrared value from the sensor
 
//  if(irValue  > 7000){                            //If a finger is detected
//     display.clearDisplay();                      //Clear the display
//     //display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE);       //Draw the first bmp  picture (little heart)
//     display.drawBitmap(5, 5, logo16_glcd_bmp, 24, 21, WHITE);
//     display.setTextSize(2);              
//     display.setTextColor(WHITE);  
//     display.setCursor(50,0);                
//     display.println("BPM");             
//     display.setCursor(50,18);                
//     display.println(beatAvg);  
//     display.display();
    
//  if (checkForBeat(irValue) == true){                        //If  a heart beat is detected
//     display.clearDisplay();                                //Clear  the display
//     //display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Draw  the second picture (bigger heart)
//     display.drawBitmap(0, 0, logo16_glcd_bmp, 32, 32, WHITE);
//     display.setTextSize(2);                                //And  still displays the average BPM
//     display.setTextColor(WHITE);             
//     display.setCursor(50,0);                
//     display.println("BPM");             
//     display.setCursor(50,18);                
//     display.println(beatAvg); 
//     display.display();

//     long delta = millis()  - lastBeat;       //Measure duration between two beats
//     lastBeat  = millis();                    // Update lastBeat to the current time

//     beatsPerMinute = 60 / (delta / 1000.0);      //Calculating  the BPM

//     if (beatsPerMinute < 255 && beatsPerMinute > 20)    // Ensure BPM is within a reasonable range before updating the rates array
//       rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
//       rateSpot %= RATE_SIZE; // Wrap the rateSpot index to keep it within the bounds of the rates array

//       // Compute the average of stored heart rates to smooth out the BPM
//       beatAvg = 0;
//       for (byte x = 0 ; x < RATE_SIZE  ; x++)
//         beatAvg += rates[x];
//       beatAvg /= RATE_SIZE;
//     }
//   }

// }
//   if (irValue < 7000){     //If no finger is detected it inform  the user and put the average BPM to 0 or it will be stored for the next measure
//      beatAvg=0;
//      display.clearDisplay();
//      display.setTextSize(1);                    
//      display.setTextColor(WHITE);             
//      display.setCursor(30,5);                
//      display.println("Please Place "); 
//      display.setCursor(30,15);
//      display.println("your finger ");  
//      display.display();
//      noTone(3);
//      }

// }
