#include <Tlc5940.h>

void setup()
{
  SerialUSB.begin(9600);
  Tlc.init(RIGHT_PWM,0); // initialise TLC5940 and set all channels off
  Tlc.init(LEFT_PWM,0); // initialise TLC5940 and set all channels off
}
 
void loop()
{
  Tlc.set(LEFT_PWM,3, 0);
  Tlc.set(RIGHT_PWM,3, 0);
  Tlc.update();
  delay(2000);

  Tlc.set(LEFT_PWM,3, 0);
  Tlc.set(RIGHT_PWM,3, 4095);
  Tlc.update();
  delay(2000);
}
