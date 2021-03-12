

// Recurring Command: 'od_ledr,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,_!"
// Immediate Command: 'od_ledi,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,_!"
// Acknowledgement to Run: "od_leda,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,_!"

#include <evolver_si.h>
#include <Tlc5940.h>

// String Input
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean serialAvailable = true;  // if serial port is ok to write on

// Mux Shield Components and Control Pins
int s0 = 7, s1 = 8, s2 = 9, s3 = 10, SIG_pin = A0;
int num_vials = 16;
int active_vial = 0;
int output[] = {60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000};
int Input[] = {4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095};

//General Serial Communication
String comma = ",";
String end_mark = "end";


// LED Settings
String led_address = "od_led";
evolver_si led("od_led", "_!", num_vials+1); // 17 CSV-inputs from RPI
boolean new_LEDinput = false;
int saved_LEDinputs[] = {4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095};

// bubble Settings
String bubble_address = "bubble";
String bubble_status = "ON";
unsigned long time1;
unsigned long time2;
unsigned long time3;


void setup() {
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);


  analogReadResolution(16);
  Tlc.init(RIGHT_PWM | LEFT_PWM, 4095);  
  Serial1.begin(9600);
  SerialUSB.begin(9600);
  // reserve 1000 bytes for the inputString:
  inputString.reserve(1000);
  while (!Serial1);

  for (int i = 0; i < num_vials; i++) {
    Tlc.set(LEFT_PWM, i, 4095 - Input[i]);
    Tlc.set(RIGHT_PWM, i, 4095);
  }
  while(Tlc.update());

}

void loop() {
  serialEvent();
  if (stringComplete) {
    if (inputString == "bubble,ON!"){
      SerialUSB.println("Turning bubbles ON");
      for (int i = 0; i < num_vials; i++) {
        Tlc.set(RIGHT_PWM, i, 1000);
      }
      while(Tlc.update());
      time1 = millis();
      time3 = time1 - time2;
      SerialUSB.print("Time since OFF command: ");
      SerialUSB.println(time3);
      SerialUSB.println("");
      }
    if (inputString == "bubble,OFF!"){
      SerialUSB.println("Turning bubbles OFF");
      for (int i = 0; i < num_vials; i++) {
        Tlc.set(RIGHT_PWM, i, 4095);
      }
      while(Tlc.update());
      time2 = millis();
      time3 = time2 - time1;
      SerialUSB.print("Time since ON command: ");
      SerialUSB.println(time3);
      }

    SerialUSB.println(inputString);
    led.analyzeAndCheck(inputString);

    
    // Clear input string, avoid accumulation of previous messages
    inputString = "";
    
    
    
    // LED Logic
    if (led.addressFound) {
      if (led.input_array[0] == "i" || led.input_array[0] == "r") {
        SerialUSB.println("Saving LED Setpoints");
        for (int n = 1; n < num_vials+1; n++) {
          saved_LEDinputs[n-1] = led.input_array[n].toInt();
        }
        
        SerialUSB.println("Echoing New LED Command");
        new_LEDinput = true;
        echoLED();
        
        SerialUSB.println("Waiting for OK to execute...");
      }
      if (led.input_array[0] == "a" && new_LEDinput) {
        update_LEDvalues();
        SerialUSB.println("Command Executed!");
        new_LEDinput = false;       
        
      }


      led.addressFound = false;
      inputString = "";
    }

    // Clears strings if too long
    // Should be checked server-side to avoid malfunctioning
    if (inputString.length() > 2000){
      SerialUSB.println("Cleared Input String");
      inputString = "";
    }
  }

  // clear the string:
  stringComplete = false;
}

void serialEvent() {
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    inputString += inChar;
    if (inChar == '!') {
      stringComplete = true;
      break;
    }
  }
}

void echoLED() {
  digitalWrite(12, HIGH);
  
  String outputString = led_address + "e,";
  for (int n = 1; n < num_vials+1 ; n++) {
    outputString += led.input_array[n];
    outputString += comma;
  }
  outputString += end_mark;
  delay(100);
  if (serialAvailable) {
    SerialUSB.println(outputString);
    Serial1.print(outputString);
  }  
  delay(100);
  digitalWrite(12, LOW);
}

void update_LEDvalues() {
  for (int i = 0; i < num_vials; i++) {
    Tlc.set(LEFT_PWM, i, 4095 - saved_LEDinputs[i]);
  }
  while(Tlc.update());
}
