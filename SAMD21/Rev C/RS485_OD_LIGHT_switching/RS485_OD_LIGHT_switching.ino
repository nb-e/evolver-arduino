  // Recurring Command: 'od_135r,1000,_!'
// Immediate Command: 'od_135i,1000,_!'
// Acknowledgement to Run: 'od_135a,1000,_!'


// Recurring Command: 'lightr,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,_!"
// Immediate Command: 'lighti,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,_!"
// Acknowledgement to Run: "lighta,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,_!"

#include <evolver_si.h>
#include <Tlc5940.h>

// String Input
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean serialAvailable = true;  // if serial port is ok to write on

// Mux Shield Components and Control Pins
int s0 = 7, s1 = 8, s2 = 9, s3 = 10, SIG_pin = A0;
int num_vials = 16;
int mux_readings[16]; // The size Assumes number of vials
int active_vial = 0;
int PDtimes_averaged = 50; //changed so read_MuxShield wouldn't take so long
int output[] = {60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000,60000};
int Input[] = {4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095};

//General Serial Communication
String comma = ",";
String end_mark = "end";

// Photodiode Serial Communication
int expected_PDinputs = 2;
String photodiode_address = "od_135";
evolver_si in("od_135", "_!", expected_PDinputs); //2 CSV Inputs from RPI
boolean new_PDinput = false;
int saved_PDaveraged = 100; // saved input from Serial Comm. ****Not used***


// LIGHT Settings
String led_address = "light";
evolver_si led("light", "_!", num_vials+1); // 17 CSV-inputs from RPI
boolean new_LEDinput = false;
int saved_LEDinputs[] = {4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095};


// Light Pausing for OD Reading
unsigned long startMillis;  //start of this period
unsigned long currentMillis; //current period length
unsigned long exptStartMillis; //arduino start time
const unsigned long period = 10000;  //number of milliseconds to wait between OD readings
int LED_save[] = {4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095,4095};
unsigned long time1;
unsigned long time2;
unsigned long time3;

// bubble Pausing for OD Reading
String bubble_address = "bubble";
String bubble_status = "ON";

void setup() {
  startMillis = millis();  //initial start time
  
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);


  // Set up Mux
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(SIG_pin, INPUT);

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  analogReadResolution(16);
  Tlc.init(LEFT_PWM,4095);
  Serial1.begin(9600);
  SerialUSB.begin(9600);
  // reserve 1000 bytes for the inputString:
  inputString.reserve(1000);
  while (!Serial1);

  for (int i = 0; i < num_vials; i++) {
    Tlc.set(LEFT_PWM, i, 4095 - Input[i]);
  }
  while(Tlc.update());

}


///// How the code was working:
///// 1. Constantly taking OD
/////  - by looping and running read_MuxShield
/////  - Each loop read one vial
///// 2. Setting light values  and outputting OD whenever the correct string was found via serial
/////
///// How it's working now:
///// 1. Take OD every "period" milliseconds
/////  - Save light values
/////  - Turn off light
/////  - Take OD for all vials
/////  - Turn on light
///// 2. This causes it to miss acknowledge commands from the server.
/////  - Therefore new light values from the server will not update the PWM board
/////  
/////  
/////  

 
void loop() {
  serialEvent();
//  if (stringComplete){
//    SerialUSB.println("~~~~String Completed~~~~");
//  }
  currentMillis = millis();
  if (currentMillis - startMillis >= period){
//     //Only read OD if period has elapsed, so we can turn off the light
//     //Necessary to get accurate OD measurement
    SerialUSB.println("Turning OFF Light");
    for (int n = 0; n < num_vials; n++) {
      LED_save[n] = saved_LEDinputs[n]; //save the light vals so when we turn on later they will be right
      saved_LEDinputs[n] = 0;
    }
    update_LEDvalues();
    SerialUSB.println("Light OFF");
    bubble_status = "OFF";
    echo_bubbles();
    delay(1000);
    SerialUSB.println("bubbles OFF");
    
    time1 = millis();
    for (int n = 0; n < num_vials; n++) {
      ////Iterate through each vial and read its OD
      read_MuxShield();
//      SerialUSB.println(active_vial);
      if (active_vial == 15){
        active_vial = 0;
      } else {
        active_vial++;
      }
    }
    
    SerialUSB.print("Reading took: ");
    SerialUSB.println(millis()-time1);
    
    SerialUSB.println("Turning ON Light");
    SerialUSB.println("Saving LED Setpoints");
    for (int n = 0; n < num_vials+1; n++) {
      saved_LEDinputs[n] = LED_save[n];
    }
    update_LEDvalues();
    SerialUSB.println("Light ON");
    
    bubble_status = "ON";
    echo_bubbles();
    SerialUSB.println("bubbles ON");
    startMillis = millis();  //start time of the current period
  }
  
  if (stringComplete) {
    SerialUSB.println("Input String Found:"+inputString);
    in.analyzeAndCheck(inputString);
    led.analyzeAndCheck(inputString);

    // Clear input string, avoid accumulation of previous messages
    inputString = "";

    
    // Photodiode Logic
    if (in.addressFound) {
      if (in.input_array[0] == "i" || in.input_array[0] == "r") {
        
        SerialUSB.println("Echoing New PD Command");
        new_PDinput = true;
        dataResponse();

        SerialUSB.println("Waiting for OK to execute...");
      }
      if (in.input_array[0] == "a" && new_PDinput) {
        
        SerialUSB.println("PD Command Executed!");
        new_PDinput = false;
      }        

      in.addressFound = false;
      inputString = "";
    }
    
    // LIGHT Logic
    // For changing set light values based off of server input
    if (led.addressFound) {
      SerialUSB.print("LIGHT address found:: ");
      for (int n = 0; n < num_vials+1; n++) {
        SerialUSB.print(led.input_array[n]);
      }
      SerialUSB.println(" ");
      SerialUSB.println("______________");
      if (led.input_array[0] == "i" || led.input_array[0] == "r") {
        SerialUSB.println("Saving LIGHT Setpoints");
        for (int n = 1; n < num_vials+1; n++) {
          saved_LEDinputs[n-1] = led.input_array[n].toInt();
        }
        SerialUSB.println("Echoing New LIGHT Command");
        new_LEDinput = true;
        echoLED();
        
        SerialUSB.println("Waiting for OK to execute...");
      }
      if (led.input_array[0] == "a" && new_LEDinput) {
//        if (new_LEDinput) {
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

void echo_bubbles() {
  digitalWrite(12, HIGH);
  
  String outputString = bubble_address+comma+bubble_status;
  outputString += "!";
  delay(100);
  if (serialAvailable) {
    SerialUSB.println(outputString);
    Serial1.print(outputString);
  }  
  delay(100);
  digitalWrite(12, LOW);
}


void update_LEDvalues() {
  SerialUSB.println(" ");
  SerialUSB.println("______________");
  SerialUSB.print("LIGHT VALUES UPDATE::");
  for (int i = 0; i < num_vials; i++) {
    Tlc.set(LEFT_PWM, i, 4095 - saved_LEDinputs[i]);
    SerialUSB.print(saved_LEDinputs[i]);
    SerialUSB.print(",");
  }
  SerialUSB.println("");
  while(Tlc.update());
}


int dataResponse (){
  digitalWrite(12, HIGH);
  String outputString = photodiode_address + "b,"; // b is a broadcast tag
  for (int n = 0; n < num_vials; n++) {
    outputString += output[n];
    outputString += comma;
  }
  outputString += end_mark;

  delay(100); // important to make sure pin 12 flips appropriately
  
  SerialUSB.println(outputString);
  Serial1.print(outputString); // issues w/ println on Serial 1 being read into Raspberry Pi

  delay(100); // important to make sure pin 12 flips appropriately

  digitalWrite(12, LOW);
}

void read_MuxShield() {
  //// Take a bunch OD readings for the active_vial and average them
  unsigned long mux_total=0;
  for (int h=0; h<(PDtimes_averaged); h++){
    mux_total = mux_total + readMux(active_vial);
  output[active_vial] = mux_total / PDtimes_averaged;
  }
}

int readMux(int channel){
  int controlPin[] = {s0, s1, s2, s3};

  int muxChannel[16][4]={
    {0, 0, 0, 0}, //channel 0; Vial 0
    {1, 1, 0, 0}, //channel 3; Vial 1
    {1, 0, 0, 0}, //channel 1; Vial 2
    {0, 1, 0, 0}, //channel 2; Vial 3
    {0, 0, 1, 0}, //channel 4; Vial 4
    {1, 1, 1, 0}, //channel 7; Vial 5
    {1, 0, 1, 0}, //channel 5; Vial 6
    {0, 1, 1, 0}, //channel 6; Vial 7
    {0, 0, 0, 1}, //channel 8; Vial 8
    {1, 1, 0, 1}, //channel 11; Vial 9
    {1, 0, 0, 1}, //channel 9; Vial 10
    {0, 1, 0, 1}, //channel 10; Vial 11
    {0, 0, 1, 1}, //channel 12; Vial 12
    {1, 1, 1, 1}, //channel 15; Vial 13
    {1, 0, 1, 1}, //channel 13; Vial 14
    {0, 1, 1, 1}, //channel 14; Vial 15
  };

  //loop through the 4 sig
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(SIG_pin);

  //return the value
  return val;
}
