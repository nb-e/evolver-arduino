#include <Tlc5940.h>

void setup()
{
  SerialUSB.begin(9600);
  Tlc.init(LEFT_PWM,4095); // initialise TLC5940 and set all channels off
}
 
void loop()
{

  Tlc.set(LEFT_PWM,3, 0);
  Tlc.update();
  delay(1000);


  Tlc.set(LEFT_PWM,3, 4095);
  Tlc.update();
  delay(1000);
}
