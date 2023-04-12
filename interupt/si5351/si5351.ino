#include <Adafruit_SI5351.h>

Adafruit_SI5351 clockgen = Adafruit_SI5351();

/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/
void setup(void)
{
  Serial.begin(115200);
  Serial.println("Si5351 Clockgen Test"); Serial.println("");

  /* Initialise the sensor */
  if (clockgen.begin() != ERROR_NONE)
  {
    /* There was a problem detecting the IC ... check your connections */
    Serial.print("Ooops, no Si5351 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  Serial.println("OK!");

  /* INTEGER ONLY MODE --> most accurate output */
  /* Setup PLLA to integer only mode @ 900MHz (must be 600..900MHz) */
  /* Set Multisynth 0 to 112.5MHz using integer only mode (div by 4/6/8) */
  /* 25MHz * 36 = 900 MHz, then 900 MHz / 8 = 112.5 MHz */
//  Serial.println("Set PLLA to 900MHz");
//  clockgen.setupPLLInt(SI5351_PLL_A, 36);
//  Serial.println("Set Output #0 to 112.5MHz");
//  clockgen.setupMultisynthInt(0, SI5351_PLL_A, SI5351_MULTISYNTH_DIV_8);

  /* FRACTIONAL MODE --> More flexible but introduce clock jitter */
  /* Setup PLLB to fractional mode @400MHz (XTAL * 16 + 0/1) */
  /* Setup Multisynth 1 to 250kHz (PLLB/1600) div range(4-900)*/
  clockgen.setupPLL(SI5351_PLL_B, 16, 0, 1);
  Serial.println("Set Output #1 to 500kHz");
  clockgen.setupMultisynth(1, SI5351_PLL_B, 800, 0, 1);

  /* Multisynth 2 is not yet used and won't be enabled, but can be */
  /* Use PLLB @ 616.66667MHz, then divide by 900 -> 685.185 KHz */
  /* then divide by 64 for 10.706 KHz */
  /* configured using either PLL in either integer or fractional mode */

//  Serial.println("Set Output #2 to 10.706 KHz");
//  clockgen.setupMultisynth(2, SI5351_PLL_B, 900, 0, 1);
//  clockgen.setupRdiv(2, SI5351_R_DIV_64);

// initialize the pushbutton pin as an input:
  pinMode(2, INPUT);
  // Attach an interrupt to the ISR vector
  attachInterrupt(2, pin_ISR, RISING);

  /* Enable the clocks */
  clockgen.enableOutputs(true);
}
int counter = 0;
/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete (your own code
    should go here)
*/
/**************************************************************************/
void loop(void)
{
if (counter % 500000 == 0){
  Serial.println(counter);
  Serial.println(millis());
  Serial.println();
}
}
void pin_ISR() {
  counter += 1;
}