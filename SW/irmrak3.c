/**** IR Mrakomer3 ****/
#define VERSION "3.1"
#define ID "$Id$"
#include "irmrak3.h"

#define  SA          0x00     // Slave Address (0 for single slave / 0x5A<<1 default)
#define  RAM_Access  0x00     // RAM access command
#define  RAM_Tobj1   0x07     // To1 address in the eeprom
#define  RAM_Tamb    0x06     // Ta address in the eeprom
#define  HEATING     PIN_A2   // Heating for defrosting
#define  MAXHEAT     60       // Number of cycles for heating

char  VER[4]=VERSION;
char  REV[50]=ID;

int8  heat;
int1  flag;

#INT_RDA
rs232_handler()
{
   char ch;

   if (getc()=='h')
   {
      heat=0;           // Need warmer
   }
   else
   {
      heat=MAXHEAT;     // Stop heating
   };

   flag=TRUE;
}


unsigned char PEC_calculation(unsigned char pec[]) // CRC calculation
{
   unsigned char   crc[6];
   unsigned char   BitPosition=47;
   unsigned char   shift;
   unsigned char   i;
   unsigned char   j;
   unsigned char   temp;

   do
   {
      crc[5]=0;            /* Load CRC value 0x000000000107 */
      crc[4]=0;
      crc[3]=0;
      crc[2]=0;
      crc[1]=0x01;
      crc[0]=0x07;
      BitPosition=47;         /* Set maximum bit position at 47 */
      shift=0;

      //Find first 1 in the transmited message
      i=5;               /* Set highest index */
      j=0;
      while((pec[i]&(0x80>>j))==0 && i>0)
      {
         BitPosition--;
         if(j<7)
         {
            j++;
         }
         else
         {
            j=0x00;
            i--;
         }
      }/*End of while */

      shift=BitPosition-8;   /*Get shift value for crc value*/


      //Shift crc value
      while(shift)
      {
         for(i=5; i<0xFF; i--)
         {
            if((crc[i-1]&0x80) && (i>0))
            {
               temp=1;
            }
            else
            {
               temp=0;
            }
            crc[i]<<=1;
            crc[i]+=temp;
         }/*End of for*/
         shift--;
      }/*End of while*/

      //Exclusive OR between pec and crc
      for(i=0; i<=5; i++)
      {
         pec[i] ^=crc[i];
      }/*End of for*/
   } while(BitPosition>8);/*End of do-while*/

   return pec[0];
}/*End of PEC_calculation*/


int16 ReadTemp(int8 addr, int8 select)    // Read sensor RAM
{
   unsigned char arr[6];         // Buffer for the sent bytes
   int8 crc;                     // Readed CRC
   int16 temp;                   // Readed temperature

   disable_interrupts(GLOBAL);
   i2c_stop();
   i2c_start();
   i2c_write(addr);
   i2c_write(RAM_Access|select);  // Select the teperature sensor in device
   i2c_start();
   i2c_write(addr);
   arr[2]=i2c_read(1);        // lo
   arr[1]=i2c_read(1);        // hi
   temp=MAKE16(arr[1],arr[2]);
   crc=i2c_read(0);           //crc
   i2c_stop();
   enable_interrupts(GLOBAL);

   arr[5]=addr;
   arr[4]=RAM_Access|select;
   arr[3]=addr;
   arr[0]=0;
   if (crc != PEC_calculation(arr)) temp=0; // Calculate and check CRC

   return temp;
}

void main()
{
   unsigned int16 n, temp, tempa;
   signed int16 ta, to;

   output_low(HEATING);                 // Heating off
   setup_wdt(WDT_2304MS);               // Setup Watch Dog
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   setup_oscillator(OSC_4MHZ|OSC_INTRC,+2);

   delay_ms(1000);
   printf("\n\r* Mrakomer %s (C) 2007 KAKL *\n\r",VER);   // Welcome message
   printf("* %s *\n\r",REV);
   printf("<#seq.> <ambient temp.> <space temp.> <status>\n\r\n\r");
   tempa=ReadTemp(SA, RAM_Tamb);       // Dummy read
   temp=ReadTemp(SA, RAM_Tobj1);

   n=0;
   heat=MAXHEAT;
   flag=FALSE;

   enable_interrupts(GLOBAL);
   enable_interrupts(INT_RDA);

   while(TRUE)
   {

      if (flag)
      {
         n++;        // Increment the number of measurement

         tempa=ReadTemp(SA, RAM_Tamb);       // Read temperatures from sensor
         temp=ReadTemp(SA, RAM_Tobj1);

         to=(signed int16)(temp*2-27315);
         ta=(signed int16)(tempa*2-27315);

         printf("%Lu %.1g %.1g ",n,(float)ta/100,(float)to/100);

         if((0==tempa)||(0==temp))           // Transfer error?
         {
            printf("-1\n\r");
            heat=MAXHEAT;
         }
         else
         {
            if (heat>=MAXHEAT)               // Active heating?
            {
               printf("0\n\r");
            }
            else
            {
               printf("1\n\r");
            }
         }

         flag=FALSE;
      };

      if (heat>=MAXHEAT)                     // Continue heating?
      {
         output_low(HEATING);
      }
      else
      {
         output_high(HEATING);
         heat++;
      }
      restart_wdt();
      delay_ms(1000);
   }
}

