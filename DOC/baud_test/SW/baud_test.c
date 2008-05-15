/**** baud test for IRMRAK ****/

#include "baud_test.h"

#define HEATING    PIN_A2 

volatile int1  flag_temp;
volatile int8  ostun;

#INT_RDA
rs232_handler()
{
   char ch;

   switch (getc())
   {
      case 'A': setup_oscillator(OSC_4MHZ|OSC_INTRC,++ostun);break;
      case 'B': setup_oscillator(OSC_4MHZ|OSC_INTRC,--ostun);break;
      case 'C': if (flag_temp) flag_temp = 0;else flag_temp=1;break;
   }
}

void main()
{
   output_low(HEATING);                 // Heating off
   setup_wdt(WDT_2304MS);               // Setup Watch Dog
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   setup_oscillator(OSC_4MHZ|OSC_INTRC,0);

   delay_ms(1000);
   ostun=0;
   flag_temp=0;

   //enable_interrupts(GLOBAL);
   //enable_interrupts(INT_RDA);
   
   while(TRUE)
   {
      while (flag_temp)
      {
         printf("\nostune\r%d",ostun);
         delay_ms(500);
      }
      putc(0x00);
   }
}

