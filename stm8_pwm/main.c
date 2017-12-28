
#include <iostm8s103f3.h>
#include <intrinsics.h>
//fsys = 2M hz
typedef unsigned char u8;
typedef unsigned int u16;

enum palette{
	RED=1,
	BLUE,
	GREEN,
	UNKONW1,
	UNKONW2,
	UNKONW3,
	UNKONW4,
};
 
const unsigned char duty_table[17]={0,4,6,8,10,12,14,16,18,21,23,25,27,29,31,33,50}; //包括最低和最高15+2
//7-4 11-6 15%-8，19%-10，23%-12，27%-14，
//32%-16，36%-18，41%-21，45%-23，50%-25，54%-27，58%-29，
//62%-31，66%-33

const unsigned char duty_table_r[7]={0,8,14,21,29,33,50}; 
const unsigned char duty_table_g[7]={0,8,14,21,29,33,50}; 
const unsigned char duty_table_b[7]={0,8,14,21,29,33,50}; 

u8 brightness1 =1;
enum palette color1 = RED;
//delay
void delay(unsigned char time)
{
  unsigned char i;
  while(time--)
  {
    for(i=0;i<200;i++);
  }
}
//三色灯
void three_color(u8 red, u8 blue, u8 green)
{

  u8 tmp_r=red,tmp_g=green,tmp_b=blue;
  u16 duty;
  
  if(red >7 || blue >7 || green >7)
	  return;
  
  duty = duty_table_g[tmp_g];
  TIM2_CCR1H = duty >> 8; //green
  TIM2_CCR1L = (duty & 0x00ff);
  
  duty = duty_table_b[tmp_b];
  TIM2_CCR2H = duty >> 8; //blue
  TIM2_CCR2L = (duty & 0x00ff);
  
  duty = duty_table_r[tmp_r];
  TIM2_CCR3H = duty >> 8; //red
  TIM2_CCR3L = (duty & 0x00ff);
}

void set_color(enum palette color) //调节色号
{
  switch(color)
  {
  case RED:
	three_color(brightness1,0,0);
    break;//red
  case BLUE: 
	three_color(0,brightness1,0);
	break;//green
  
  case GREEN: 
	three_color(0,0,brightness1);
	break;//blue
  case UNKONW1:
	three_color(0,brightness1,brightness1);
	break;
  case UNKONW2: 
	three_color(brightness1,brightness1,0);
	break;
  case UNKONW3: 
	three_color(brightness1,brightness1,brightness1);
	break;
  case UNKONW4: 
	three_color(brightness1,0,brightness1);
	break;
  default:break;
  }
  
}


void main()
{
//--------------
  CLK_PCKENR1 = 0xff;
  PD_DDR = 0xff;
  PD_CR1 = 0xff;
  PD_CR2 = 0xff;
  
  PC_DDR = 0xff & ~(1<<3);//bit3/bit4 as input
  PC_DDR &= ~(1<<4);
  
  PC_CR1 = 0xff;
  PC_CR2 = 0xff;
  
  PA_DDR = 0xff;
  PA_CR1 = 0xff;
  PA_CR2 = 0xff;
  
  TIM2_CCMR1 = 0x68;
  TIM2_CCMR2 = 0x68;
  TIM2_CCMR3 = 0x68;
    
  TIM2_ARRH = 0x00;
  TIM2_ARRL = 0x32; //50

  TIM2_CCR1H = 0x00;
  TIM2_CCR1L = 0x04;  //7-4 11-6 15%-8，19%-10，23%-12，27%-14，
                      //32%-16，36%-18，41%-21，45%-23，50%-25，54%-27，58%-29，
                      //62%-31，66%-33

  
  TIM2_CCR2H = 0x00;
  TIM2_CCR2L = 0x04;
  
  TIM2_CCR3H = 0x00;
  TIM2_CCR3L = 0x04;
  
  TIM2_CCER1 = 0x11;
  TIM2_CCER2 = 0x01;
  TIM2_CR1 = 0x81;
  //------------------
  
  set_color(color1);
   while(1)
  {
    u8 tmp;
    tmp = PC_IDR;
    tmp &= (1<<4);
    if(!(tmp & (1<<4)))
    {
      delay(200);
	  delay(200);
	  delay(200);
      if(!(tmp & (1<<4)))
      {
        if(++brightness1 > 7)
			brightness1 = 0;
		set_color(color1);
      }
    }
    
    tmp = PC_IDR;
    tmp &= (1<<3);
    if(!(tmp & (1<<3)))
    {
		delay(200);
		delay(200);
		delay(200);
		if(!(tmp & (1<<3)))
		{
                  if(++color1 > 7)
                      color1 = RED;
                set_color(color1);
		}
    }
    
  }
  
}