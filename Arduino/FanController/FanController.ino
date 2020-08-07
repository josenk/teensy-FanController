#include <stdio.h>
#include <string.h>
#include <EEPROM.h>
#define USBSERIAL Serial

// Tunables
#define eepromOffset 0
#define isDebugOffset 3
#define isCelsiusOffset 4
#define fanSpeedOffset 10

#define versionMajor 1
#define versionMinor 0
#define versionPatch 2
#define pinBut1 0
#define tempIterations 10
#define numTemps 6
#define numFans 6




// Global Variables
int      led = 13;
bool     isCelsius = true;
byte     tempSensor[numTemps] = {0,1,2,3,4,5};     //  Analog Out, Uses Digital Pins 14-19
uint8_t  pwmOut[numFans] = {3,4,5,6,9,10};         //  PWM Out  (Set fan speed)
uint8_t  pwmIn[numFans] = {0,1,2,7,8,11};          //  PWM In   (Get fan speed)

int curTemp[numTemps];                   // Current Temp readings
int currentFanSpeed[numFans];            // Current Fan speed readings

// Misc
bool    isDebug = 0;


// the setup routine runs once when you press reset:
void setup() {

  uint8_t C;              // Misc Counter

  // Set pinModes
  pinMode(led, OUTPUT);
  pinMode(pinBut1,INPUT_PULLUP);
  for (C=0; C <numTemps; C++) {
    pinMode(tempSensor[C], INPUT);
  }
  
  for (C=0; C <numFans; C++) {
    pinMode(pwmIn[C], INPUT_PULLUP);
    pinMode(pwmOut[C], OUTPUT);
  }

  analogReadAveraging(5);
  analogReadRes(10);
  
  
  // initialize serial communications.
  Serial.begin(9600);

  // Initialize eeprom on "first boot" OR Major version upgrade OR pressing the Button on bootup.
  // Write in eeprom the version number.
  if (EEPROM.read(eepromOffset) != versionMajor ||
       digitalRead(pinBut1) == 0 ) {

    setDefaults();    
  }
  
  // Init Fan speeds
  for (C=0; C < numFans; C++)
  {
    currentFanSpeed[C] = 0;
    analogWrite(pwmOut[C], EEPROM.read(eepromOffset + fanSpeedOffset + C) * 2.55);
  }

  isDebug = EEPROM.read(eepromOffset + isDebugOffset);
  isCelsius = EEPROM.read(eepromOffset + isCelsiusOffset);
}


//  The loop routine runs over and over again forever:
void loop() {

  int     C, CC;              // Misc Counters
  char    usbOutString[255];  // string to send to USB
  char    usbInString[255];   // string in from USB
  uint8_t bytecount = 0;

  char*   argsv[10];
  uint8_t argc=0;
  char*   token = NULL;


  //  Read Temp sensors
  for (C=0; C < numTemps; C++) {
    curTemp[C] = getTemp(C);
  }

  //  Read Fan speeds
  getFanSpeed();


  //
  //  If we received something from USB Serial
  //
  while (Serial.available() && bytecount < 250 ) {
    usbInString[bytecount++] = (char)Serial.read();
  }
  usbInString[bytecount+1] = '\0';

  // Tokenize rx string from USB
  if ( bytecount > 0){
    // First Token
    token = strtok(usbInString, " \n\r");

    while( token != NULL && argc < 8) 
    {
      argsv[argc++] = token;
      token = strtok(NULL, " \n\r");
    }

    //
    //  Enable/Disable Debug
    //
    if (String(argsv[0]).equalsIgnoreCase("debug") && argc > 0) {
      if (String(argsv[1]).equalsIgnoreCase("true")) {
        isDebug = true;
        
      } else if (String(argsv[1]).equalsIgnoreCase("false")) {
        isDebug = false;
      }
        EEPROM.update(eepromOffset + isDebugOffset, isDebug);
    }
    
    //
    //  Get
    //
    if (String(argsv[0]).equalsIgnoreCase("get") && argc > 0 ) {

      //  Get Temp
      if (String(argsv[1]).equalsIgnoreCase("temp") && argc > 1 ) {

        // Get Temp All
        if (String(argsv[2]).equalsIgnoreCase("all")) {
          for (C=0; C <numTemps; C++) {
            sprintf(usbOutString, "Temp%d=%s\n", C, calcTemp(curTemp[C]));
            Serial.print(usbOutString);
          } 
          
        } else {

          // Get Temp #
          C = (int)strtol(argsv[2],NULL,10);
          if (C >= 0 && C < numTemps) {
            sprintf(usbOutString, "Temp%d=%s\n", C, calcTemp(curTemp[C]));
            Serial.print(usbOutString);
            
          } else {
            Serial.printf("Temp probe ID must be in range 0 - %d\n", numTemps - 1);
          }
        }
      }

      // Get Fan
      if (String(argsv[1]).equalsIgnoreCase("fan") && argc > 1 ) {

        // Get Fan All
        if (String(argsv[2]).equalsIgnoreCase("all")) {
          for (C=0; C <numFans; C++) {
            sprintf(usbOutString, "FanSpeed%d=%d\n", C, currentFanSpeed[C] );
            Serial.print(usbOutString); 
          } 
          
        } else if (String(argsv[2]).equalsIgnoreCase("eeprom")) {
          for (C=0; C <numFans; C++) {
            sprintf(usbOutString, "eeprom%d=%d\n", C, EEPROM.read(eepromOffset + fanSpeedOffset + C));
            Serial.print(usbOutString); 
          }
          
        } else
        {

          // Get Fan #
          C = (int)strtol(argsv[2],NULL,10);
          if (C >= 0 && C < numFans) {
            sprintf(usbOutString, "FanSpeed%d=%d\n", C, currentFanSpeed[C] );
            Serial.print(usbOutString);
            
          } else {
            Serial.printf("Fan ID must be in range 0 - %d\n", numFans - 1);
          }
        }
      }
    }


    // Set
    if (String(argsv[0]).equalsIgnoreCase("set")) {
      if (String(argsv[1]).equalsIgnoreCase("defaults")) {
        setDefaults();
        
      } else if (String(argsv[1]).equalsIgnoreCase("celsius") && argc > 1 ) {
        isCelsius = true;
        Serial.printf("Set scale to Celsius\n");
        
      } else if (String(argsv[1]).equalsIgnoreCase("fahrenheit") && argc > 1 ) {
        isCelsius = false;
        Serial.printf("Set scale to Fahrenheit\n");
        
      } else if (String(argsv[1]).equalsIgnoreCase("fan") && argc > 1 ) {
        // Get Fan #
        C = (int)strtol(argsv[2],NULL,10);
      
        if (C >= 0 && C < numFans) {
          CC = (int)strtol(argsv[3],NULL,10);
          if (CC >= 0 && CC <= 100) {
          
            Serial.printf("Set Fan %d %d\%\n", C, CC);
            analogWrite(pwmOut[C], int(CC * 2.55));
            
          } else {
            Serial.printf("Fan RPM (percent) must be in range 0-100\n");
          }
          
        } else {
          Serial.printf("Fan ID must be in range 0 - %d\n", numFans - 1);
        }
      }
    }

     // Save (to eeprom)
    if (String(argsv[0]).equalsIgnoreCase("save")) {
      if (String(argsv[1]).equalsIgnoreCase("celsius") && argc > 1 ) {
        isCelsius = true;
        EEPROM.update(eepromOffset + isCelsiusOffset, isCelsius);
        Serial.printf("Save scale to Celsius\n");
        
      } else if (String(argsv[1]).equalsIgnoreCase("fahrenheit") && argc > 1 ) {
        isCelsius = false;
        EEPROM.update(eepromOffset + isCelsiusOffset, isCelsius);
        Serial.printf("Save scale to Fahrenheit\n");
        
      } else if (String(argsv[1]).equalsIgnoreCase("fan") && argc > 1 ) {
        // Get Fan #
        C = (int)strtol(argsv[2],NULL,10);
      
        if (C >= 0 && C < numFans) {
          CC = (int)strtol(argsv[3],NULL,10);
          if (CC >= 0 && CC <= 100) {
          
            Serial.printf("Save Fan %d %d\%\n", C, CC);
            EEPROM.update(eepromOffset + fanSpeedOffset + C , CC);
            analogWrite(pwmOut[C], int(CC * 2.55));
            
          } else {
            Serial.printf("Fan RPM (percent) must be in range 0-100\n");
          }
          
        } else {
          Serial.printf("Fan ID must be in range 0 - %d\n", numFans - 1);
        }
      }
    }

    // Get Version
    if (String(argsv[0]).equalsIgnoreCase("version")) {
      Serial.printf("Version=%d.%d.%d\n", versionMajor, versionMinor, versionPatch);
    }
  }
}




//    -----------------  Functions ------------------------

void getFanSpeed(void) {
  elapsedMillis tm_SEC;
  uint32_t Counter = 0;
  uint8_t  i = 0;                // Current Fan to Read
  char    usbOutString[255];

  //  Fan related stuff...  To Calculate RPMS
  int CurrReading[numFans] = {0};
  int OldReading[numFans] = {0};
  int pwmInCounter[numFans] = {0};


  //  Look for HIGH --> LOW drops.
  while (tm_SEC < 1000 ){

    for(i=0;i < numFans; i++) {
      CurrReading[i] = digitalRead(pwmIn[i]);
      if (CurrReading[i] == LOW && OldReading[i] == HIGH) {
        pwmInCounter[i]++;
      }
      OldReading[i] = CurrReading[i];
    }
    
    Counter++;
  }
  
  for(i=0;i < numFans; i++) {
    if (pwmInCounter[i] > 2){
      currentFanSpeed[i] = (uint16_t)((float)pwmInCounter[i] / 8.0) * 60;
    } else {
      currentFanSpeed[i] = 0;
    }
  }

  if (isDebug) {
    for(i=0;i < numFans; i++) {
      sprintf(usbOutString, "FanSensor=%-d Pin=%d pwmInCounter=%-3d RPM=%-4d Counter=%lu\n", i, pwmIn[i], pwmInCounter[i], currentFanSpeed[i], Counter);
      if (i == 0) {
        Serial.print('\n');
      }
      Serial.print(usbOutString);
    }
  }

  return ;
}


int getTemp(uint8_t C) {
  uint16_t code;
  float celsius = 0;
  int   CC;

 
  for(CC=0;CC < tempIterations;CC++) {
    code = analogRead(C);
    if (code <= 289)               { celsius = celsius + ( 5 + (code - 289) / 9.82);  }
    if (code > 289 && code <= 342) { celsius = celsius + (10 + (code - 342) / 10.60); }
    if (code > 342 && code <= 398) { celsius = celsius + (15 + (code - 398) / 11.12); }
    if (code > 398 && code <= 455) { celsius = celsius + (20 + (code - 455) / 11.36); }
    if (code > 455 && code <= 512) { celsius = celsius + (25 + (code - 512) / 11.32); }
    if (code > 512 && code <= 566) { celsius = celsius + (30 + (code - 566) / 11.00); }
    if (code > 566 && code <= 619) { celsius = celsius + (35 + (code - 619) / 10.44); }
    if (code > 619 && code <= 667) { celsius = celsius + (40 + (code - 667) / 9.73);  }
    if (code > 667 && code <= 712) { celsius = celsius + (45 + (code - 712) / 8.90);  }
    if (code > 712 && code <= 752) { celsius = celsius + (50 + (code - 752) / 8.03);  }
    if (code > 752 && code <= 788) { celsius = celsius + (55 + (code - 788) / 7.13);  }
    if (code > 788 && code <= 819) { celsius = celsius + (60 + (code - 819) / 6.29);  }
    if (code > 819 && code <= 847) { celsius = celsius + (65 + (code - 847) / 5.50);  }
    if (code > 847 && code <= 870) { celsius = celsius + (70 + (code - 870) / 4.77);  }
    if (code > 870 && code <= 891) { celsius = celsius + (75 + (code - 891) / 4.12);  }
    if (code > 891 && code <= 909) { celsius = celsius + (80 + (code - 909) / 3.56);  }
    if (code > 909 && code <= 924) { celsius = celsius + (85 + (code - 924) / 3.05);  }
    if (code > 924)                { celsius = celsius + (90 + (code - 712) / 2.62);  }
  }

  //  No probe
  if (code <= 289) {
    celsius = 9999;
  }

  return int(celsius);  // This result is actually celsius * tempIterations
}


char *calcTemp(int val) {
    static char returnString[10];
    float temperature = 0;
    float float_val = val;
  
    if ((val > 0) and (val < 200)) {
        if (isCelsius == true){
            temperature = float_val / tempIterations;
        } else {
            temperature = ((float_val / tempIterations) * 1.8) + 32;
        }
        
        uint16_t Int = int(temperature);           //  The Integer part of the float
        uint16_t Dec = (temperature - Int) * 10;   //  The Decimal part of the float
        sprintf(returnString, "%d.%d", Int, Dec);
    } else {
        sprintf(returnString, "NoProbe");
    }

    return returnString;
}


void setDefaults(void)
{
  int C;

  isDebug = 0;
  
  EEPROM.update(eepromOffset,     versionMajor);
  EEPROM.update(eepromOffset + 1, versionMinor);
  EEPROM.update(eepromOffset + 2, versionPatch);
  EEPROM.update(eepromOffset + isDebugOffset, 0);       // Debug disabled by default
  EEPROM.update(eepromOffset + isCelsiusOffset, true);  // Celsius is default
    
  for (C=0; C <numFans; C++) {
    digitalWrite(led,HIGH);
    EEPROM.update(eepromOffset + fanSpeedOffset + C ,100);   // Set fans to Max (100%) by default.
    analogWrite(pwmOut[C], 255);
    delay(50);
    digitalWrite(led,LOW);
    delay(50);
  }
  Serial.print("Set Defaults...\n");
}
