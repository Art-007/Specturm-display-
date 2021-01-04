/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"

void uart_checkvoltage(double reading) {
   static double ini_volt = 0;

   double diff;
   diff = (ini_volt >= reading) ? ini_volt-reading : reading-ini_volt;

   if(diff > .001)//if voltage changes show new voltage
   {
	   uart.disp("current voltage: ");
	   uart.disp(reading,3);
	   uart.disp("\n\r");
   	   ini_volt = reading;
   }
}

void ledBlink(XadcCore *adc_p, GpoCore *led_pm, int led)
{
	int delay;
	uint16_t raw;
	double reading;

	raw = adc_p->read_raw(0);
	delay= raw >> 4;//set potentiometer value to delay of blink
	reading = adc_p->read_adc_in(0);
	uart_checkvoltage(reading);//check for current delay / voltage
	led_pm->write(1, led);
	sleep_ms(delay+50);//inputted delay starts at 50ms
	led_pm->write(0, led);
	sleep_ms(delay+50);
}

void led_chase(XadcCore *adc_p, GpoCore *led_pm, int amt ) {
   int static pos;
   int start = 0;
   int end = amt;

   for (pos = start; pos < end; pos++)
   {

	   ledBlink(adc_p, led_pm, pos);

   }
   for(pos = pos-1; pos > start ; pos--)
   {
		ledBlink(adc_p, led_pm, pos);
   }
}


void pwm_3color_led_check(PwmCore *pwm_p) {
   int i, n;
   double bright, duty;
   const double P20 = 1.2589;  // P20=100^(1/20); i.e., P20^20=100

   pwm_p->set_freq(50);
   for (n = 0; n < 3; n++) {
      bright = 1.0;
      for (i = 0; i < 20; i++) {
         bright = bright * P20;
         duty = bright / 100.0;
         pwm_p->set_duty(duty, n);
         pwm_p->set_duty(duty, n + 3);
         sleep_ms(100);
      }
      sleep_ms(300);
      pwm_p->set_duty(0.0, n);
      pwm_p->set_duty(0.0, n + 3);
   }
}

double noise_correction(double reading)
{
	static double ini_volt = 0;

	double diff;
	diff = (ini_volt >= reading) ? ini_volt-reading : reading-ini_volt;
	if(diff > 0.002)//if voltage changes more than .002 update new value
	{
		ini_volt = reading;
		return reading;
	}
	else
		return ini_volt;
}

void intensity_change(PwmCore *pwm_p, double redInt, double greenInt, double blueInt, double inc )
{

   double redDuty, greenDuty, blueDuty;

   //red
   redDuty = redInt * inc;
   pwm_p->set_duty(redDuty, 2);
   pwm_p->set_duty(redDuty, 5);

   //green
   greenDuty = greenInt * inc;
   pwm_p->set_duty(greenDuty, 1);
   pwm_p->set_duty(greenDuty, 4);

   //blue
   blueDuty = blueInt * inc;
   pwm_p->set_duty(blueDuty, 0);
   pwm_p->set_duty(blueDuty, 3);

}

void  seg_output(XadcCore *adc_p,  SsegCore *sseg_p) {

   int i;
   double reading ;



   // reading adc
   	reading = noise_correction(adc_p->read_adc_in(0));


   	   double fa, frac, shift_left1, shift_left2, shift_left3; // absolute value of f
   	   int i_part, tenths, hundredths, thousandths;

   	   fa = reading;
   	   // display integer portion
   	   i_part = (int) fa; // integer part of fa
   	   frac = fa - (double) i_part;

   	   //tenths place
   	   shift_left1 = frac*10.0;//shift tenths place number to ones place
   	   tenths = (int)shift_left1 ;

   	   //hundredths place
   	   shift_left2 = (shift_left1 - tenths)*10.0; //subtract shifted value with integer to isolate decimal part of number
   	   hundredths = (int)shift_left2 ;

   	   //thousandths place
   	   shift_left3 = (shift_left2-hundredths)*10.0;
   	   thousandths = (int)shift_left3;

       //turn off led
      for (i = 0; i < 8; i++) {
         sseg_p->write_1ptn(0xff, i);
      }

         //turn off all decimal points
         sseg_p->set_dp(0x00);

        // set ones place
       sseg_p->write_1ptn(sseg_p->h2s(i_part) , 3);

        // set tenths place
       sseg_p->write_1ptn(sseg_p->h2s(tenths), 2);

       // set hundredths place
       sseg_p->write_1ptn(sseg_p->h2s(hundredths), 1);

       // set thousandths place
       sseg_p->write_1ptn(sseg_p->h2s(thousandths), 0);

     // place decimal in the 3rd position
        sseg_p->set_dp(8);
}


void colorChange(PwmCore *pwm_p, XadcCore *adc_p, SsegCore *sseg_p)
{
	double voltReading, unfiltReading;
	unfiltReading = adc_p->read_adc_in(0);
	voltReading = noise_correction(unfiltReading);

	if(voltReading >= 0 && voltReading <= 0.166)//red to yellow
	{
		intensity_change(pwm_p, 0.166, voltReading, 0, 0.4);
	}


	if(voltReading >= 0.167 && voltReading <= 0.332)//yellow to green
	{
		intensity_change(pwm_p, 0.332 - voltReading, 0.166 , 0, 0.4);
	}


	if(voltReading >= 0.333 && voltReading <= 0.498)//green to light blue
	{
		intensity_change(pwm_p, 0, 0.166, voltReading - 0.333, 0.4);
	}


	if(voltReading >= 0.499 && voltReading <= 0.664)//light blue to blue
	{
		intensity_change(pwm_p, 0, 0.664 - voltReading, 0.166, 0.4);
	}


	if(voltReading >= 0.665 && voltReading <= 0.830)//blue to magenta
	{
		intensity_change(pwm_p, voltReading - 0.655, 0, 0.166, 0.4);
	}


	if(voltReading >= 0.831 && voltReading <= 0.996)//magenta to red
	{
		intensity_change(pwm_p, 0.166, 0, 0.996 - voltReading, 0.4);
	}

	seg_output(adc_p,  sseg_p);
}



GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));


int main() {

   while (1) {
	   colorChange(&pwm, &adc, &sseg);

   } //while
} //main
