/**************************************************************************/
/*!
    @file     DACandADC.ino
    @author   Andrej Lozar, IJS
    

    This example operates MCP4725 DAC and ADS1115 ADC over I2C.
    Control is made with commands over serial communication.

    Last mod: 9/12/2020
    History:
    9.12.2020: set(x), x < 0 -> current goes to max. FIX: set to 0
    
*/
/**************************************************************************/
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_ADS1015.h>

Adafruit_MCP4725 dac;
Adafruit_ADS1115 ads(0x48);  // ADC object at I2C address 0x48 for addr pin = GND

const int ledCurrOperate = 8; // Number of the on/off pin, 1=Off

int16_t adc0;  // variables to hold ADC readings
float multiplier = 0.1875F;               // ADS1115  @ +/- 6.144V gain = 0.1875mV/step
//float multiplier = 0.125F;               // ADS1115  @ +/- 2.048V  gain = 0.125mV/step
//float multiplier = 0.0625F;               // ADS1115  @ +/- 1.024V  gain = 0.0625mV/step

float adcScale   = 4.8828F;               // Arduino 10 bit @ 5 volts   = 4.88mV/step

// Define states
const int idleState = 0;
const int measureState = 1;
const int delayState = 2;
const int setVoltageState = 3;
const int switchDacState = 4;
const int operateCurrent = 5;
const int pulseModeState = 6;

int currentState = idleState;

// Status variables
int DAC_status;
int current_status;

//Debug
int debug = 0;

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledCurrOperate, OUTPUT);
  digitalWrite(ledCurrOperate, HIGH); // 1=Off
  
  Serial.begin(115200);
  if(debug) Serial.println("Serial initialized.");

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  // Voltage values: 0-4095
  
  dac.begin(0x60);
  
  if(debug) Serial.println("DAC connected (0x60).");
  delay(1);
  dac.setVoltage(0, false);
  DAC_status = 0;
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1115
  //                                                                -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
  //ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  //ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.0078125mV
  ads.begin();  // init ADS1115 ADC

  
}

void loop(void) {    
    //Serial command
    String command;
    
    // Variables
    int VSET = 0;
    int index0, index1;
    double pulseLength = 0;
    
    String command_0 = "";
    String arg = "";
    
    
    while(1){
      switch(currentState)
      {
        case idleState :
          if(Serial.available()){
            command = Serial.readStringUntil('\n');
            command_0 = ""; // reset string
            arg = ""; // reset string
            
            index0 = command.indexOf("(");
            index1 = command.indexOf(")");
            
            if((index0 == -1) or (index1 == -1)){
              if(debug) Serial.print("Expected '(' or ')': ");
              Serial.print("-1: ");Serial.println(command);
              break;
            }
            
            for(int i = 0; i < index0; i++){
              //Serial.print(command[i]);
              command_0.concat(command[i]);
            }
            for(int i = index0+1; i < index1; i++){
              //Serial.print(command[i]);
              arg.concat(command[i]);
            }
            
            if(debug){
              Serial.print("Index: ");
              Serial.print(index0);
              Serial.print(", ");
              Serial.println(index1);
              Serial.print("Command: ");
              Serial.println(command_0);
              Serial.print("Argument: ");
              Serial.println(arg);
            }
            
            
            if(command.equals("help()")){
              Serial.println("This is ADC and DAC state machine. ATM on idle. Source: DACandADC.ino"); 
              Serial.println("Commands:");
              Serial.println("- set(X)    : Set DAC voltage to X, where X is an integer between 0 (0V) and 4095 (Vcc)");
              Serial.println("- operDAC(X)   : Turn on DAC voltage (X=1), turn off voltage (X=0)");
              Serial.println("- operCur(X)   : Turn on current source (X=1), turn off (X=0)");
              Serial.println("- measure() : Measure voltage @ A0 pin with ADC ADS1115");
              Serial.println("- debug()   : Enable/disable debug mode");
              Serial.println("- pulseMode(X) : Start pulse where X is length of the pulse");
            }
            
            else if(command_0.equals("set")){
              currentState = setVoltageState; // change state to set voltage state
            }
        
            else if(command_0.equals("operDAC")){
              currentState = switchDacState; // change state to switch DAC state
            }
            
            else if(command_0.equals("operCur")){
              currentState = operateCurrent; // change state to switch DAC state
            }
            
            else if(command.equals("measure()")){
              currentState = measureState; // change state to measure state
            }

            else if(command.equals("debug()")){
              switch(debug){
                case 0:
                  debug = 1;
                  break;
                
                case 1:
                  debug = 0;
                  break;
              }
            }

            else if(command_0.equals("pulseMode")){
              currentState = pulseModeState;
            }
            
            else{
              Serial.print("?: "); Serial.println(command); 
            }
            
          }
          break;

        case measureState :
          // read in analog inputs
          adc0 = ads.readADC_SingleEnded(0);        // read single AIN0
          Serial.println(adc0 * multiplier/1000, 5);
          currentState = idleState; // change state to idle state
          break;
        
        case setVoltageState :
          // set voltage, if DAC on, force change it
          VSET = arg.toInt();
          
          if((0 > VSET) or (VSET > 4095)){
            Serial.println("DAC off rng");
            Serial.println("set to 0");
            VSET=0;
          }
          
          if(debug) {
            Serial.print("Set voltage to ");
            Serial.println(VSET);
          }
          
          if(DAC_status) dac.setVoltage(VSET, false);
          
          currentState = idleState; // change state to idle state
          break;
          
        case switchDacState:
          DAC_status = arg.toInt();
          if(debug) Serial.print("Switching DAC "); 
          switch(DAC_status){
            case 1:
              digitalWrite(LED_BUILTIN, HIGH);
              dac.setVoltage(VSET, false);
              break;
            case 0:
              digitalWrite(LED_BUILTIN, LOW);
              dac.setVoltage(0, false);
              break;
          }
          if(debug) Serial.println(DAC_status);
          currentState = idleState; // change state to idle state
          break;

        case operateCurrent:
          current_status = arg.toInt();
          if(debug) Serial.print("Switching current "); 

          switch(current_status){
            case 1:
              digitalWrite(ledCurrOperate, LOW);
              if(debug) Serial.println("on.");
              break;
            case 0:
              digitalWrite(ledCurrOperate, HIGH); // 1 = Off
              if(debug) Serial.println("off.");
              break;
          }
          
          currentState = idleState;
          break;
          
        case pulseModeState :
          //Initialise pulse mode state
          pulseLength = arg.toDouble();
          if(debug){
            Serial.print("Starting pulse ");
            Serial.println(pulseLength);
          }

          //Set DAC
          digitalWrite(LED_BUILTIN, HIGH);
          dac.setVoltage(VSET, false);

          //Enable current
          digitalWrite(ledCurrOperate, LOW);
          if(debug) Serial.println("on.");

          //Wait, duty cycle on
          delay(pulseLength);

          //Measure voltage
          adc0 = ads.readADC_SingleEnded(0);        // read single AIN0
          
          //Disable current
          digitalWrite(ledCurrOperate, HIGH);
          if(debug) Serial.println("off.");

          //Write measured voltage to serial.
          Serial.println(adc0 * multiplier/1000, 5);
          
          //Set DAC to 0
          //digitalWrite(LED_BUILTIN, LOW);
          //dac.setVoltage(0, false);
          
          currentState = idleState;
          break;
          
        case delayState :
          delay(1);
          currentState = idleState;
          break;
          
        default :
          //undefined state
          break;
   
      }
    }
}
