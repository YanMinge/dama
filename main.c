
/*****************************************************************
Aut
*****************************************************************/
//Crystal frequency
#ifndef F_CPU 
#define F_CPU 8000000UL
#endif

//head files
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"


#define BIT(x)  1 << (x)
#define uint unsigned int
#define uchar unsigned char  

#define start 0x08
#define add_ack 0x18
#define date_ack 0x28
#define read_ack 0x50  

#define Start() (TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))  //start signal
#define Stop() (TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))   //stop signal
#define Wait() while(!(TWCR&(1<<TWINT)))                             //wait signal
#define TestACK() (TWSR&0xF8)                                                //check status
#define SetACK() (TWCR|=(1<<TWEA))                                     //set ACK
#define Writebyte(twi_d) {TWDR=(twi_d);TWCR=(1<<TWINT)|(1<<TWEN);}
#define Readbyte() (TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA))
#define tea5767_add_w 0xc0
#define tea5767_add_r 0xc1

#pragma data:data   
uint frequency_5767=8770;                                                    //Base Freq: 87.7MHZ
char frequen_temp1=0,frequen_temp2=0,frequen_temp3=0,frequen_temp4=0;
uchar i=0,flag=0,flag_key=0,add=1,flag_dis=0,flag_temp=0,temp_save=0;
uchar save_add=1;
uchar receive[2];
uchar send[2];
uint long_key=0;
uchar rx[5]={0,0,0,0,0};
uint pll;
uchar flag1=0,flag2=0,temp=0,temp1=0;
char WriteDataWord[5];
uint frequency_tab_l[]={8770,8790,8810,8830,8850,8870,0};
uint frequency_tab_h[]={10670,10690,10710,10730,10750,10770,10790,0};

//data for LCD display
void count_dis()
{  
  frequen_temp1=(frequency_5767/10)%10;
  frequen_temp2=(frequency_5767/100)%10;
  frequen_temp3=(frequency_5767/1000)%10;
  frequen_temp4=(frequency_5767/10000)%10;
}


//function:E2PROM read byte
uchar e2prom_read(uint addr)  
{
  while(EECR & (1<<EEWE));
  EEAR = addr;
  EECR|=BIT(EERE);
  return EEDR;
}


//function:E2PROM write byte
void e2prom_write(uint addr,uchar wData)
{
  while(EECR & BIT(EEWE));
  EEAR=addr;
  EEDR=wData;
  EECR|=BIT(EEMWE);
  EECR|=BIT(EEWE);
}


//function:E2PROM read string
void e2prom_reads(uint addr, char *ptr, uint size)
{
  uint i;
  for(i=0;i<size;i++)
  {
    *ptr=e2prom_read(addr+i);
    ptr ++;
    _delay_us(100);
  }
}

//function:E2PROM write string
void e2prom_writes(uint addr, uchar *ptr, uint size)
{
  uint i;
  for(i=0;i<size;i++)
  {
    e2prom_write(addr+i,*ptr);
    ptr ++;
    _delay_us(100);
  }
}

void save_channel(void)
{
  send[0]=(uchar)(frequency_5767/256);
  send[1]=frequency_5767&0x00ff;
  e2prom_writes(1,send,2);
  uart_printf("save_channel,send[0]=%d,send[1]=%d,frequency=%d\r\n",send[0],send[1],frequency_5767);
  _delay_us(100);

}


void calculate_pll(unsigned long temp)
{
  pll = (unsigned int)(((temp*10-225)*4000)/32768);
  uart_printf("calculate_pll. pll=%d",pll);
}

/*********write LCD 1602 command**********/
void write_com(uchar com)
{
  PORTA&=0xbf;                         //rs clr 0, for write
  PORTB = (PORTB & 0x0f)|com;          //write, send high 4 bit
  PORTA|=0x80;                         //set en
  PORTA&=0x7f;                         //clr en

  PORTA&=0xbf;                         //rs clr 0, for write
  PORTB = (PORTB & 0x0f)|(com<<4);     //write, send high 4 bit
  PORTA|=0x80;                         //set en
  PORTA&=0x7f;                         //clr en
}

/*********write LCD 1602 data**********/
void write_dat(uchar dat)
{
  PORTA|=0x40;                          //rs set, write data
  PORTB = (PORTB & 0x0f)|dat;           //write, send low 4 bit
  PORTA|=0x80;
  PORTA&=0x7f;

  PORTA|=0x40;                          //rs set, write data
  PORTB = (PORTB & 0x0f)|(dat<<4);      //write, send low 4 bit
  PORTA|=0x80;
  PORTA&=0x7f;
}

/*********1602 LCD init**********/
void init_1602()
{
  PORTA&=0x7f;      //en clr
  write_com(0X33);
  _delay_ms(5);
  write_com(0X32);
  _delay_ms(5);
  write_com(0X28);
  _delay_ms(5);
  write_com(0X0c);
  _delay_ms(5);
  write_com(0X06);//auto move
  _delay_ms(5);
  write_com(0X01);//auto move»
  _delay_ms(5);
}

void send_cgram(uchar len,uchar cgram_add)
{
  uchar k;
  write_com(cgram_add);
  for(k=0;k<len;k++)
  {
    //write_dat(table[k]);
  }
}

/*******************************************
function name: LCD1602_gotoXY
function: display data on LCD 1602 addr
Parameters: Row-- select row
                   Col--  select col
return: NULL
/********************************************/
void LCD1602_gotoXY(uchar Row,uchar Col)
{        
  switch (Row)         //select row
  {
    case 1:
      write_com(0x80+Col); 
      break;        
    default:
     write_com(0x80+0x40+Col); 
     break;    
   }
}

void LCD1602_disCGRAM(void)
{
  uchar i;
  LCD1602_gotoXY(1,6);
  for (i = 0;i <4;i++)
  {                 
    write_dat(i);                
  }
  LCD1602_gotoXY(2,6);
  for (i = 4;i <8;i++)
  {                 
    write_dat(i);        
  }
}

/*******************************************
function name: LCD1602_sendstr 
Function: Writes a string to the 1602 LCD
Parameters: ptString - string pointer 
Returns: NULL
/********************************************/
void LCD1602_sendstr(uchar *ptString)
{
  while((*ptString)!='\0')
  {
    write_dat(*(ptString++));
    _delay_ms(0);
  }
}

void display(uchar hang,uchar lie,uchar *str)
{
  LCD1602_gotoXY(hang,lie);
  LCD1602_sendstr(str);
}

void display_one(uchar hang,uchar lie,uchar wor)
{
  LCD1602_gotoXY(hang,lie);
  write_dat(wor);
}

/**************************************
Module name: TWI bus initialization 
Module features: TWI bus configuration, so that work is compatible with tea5767
**************************************/
void init_twi(void)
{
  TWCR = 0X00;  //close the TWI bus 
  TWBR = 0x12;  //set the transmission bit rate 
  TWSR = 0x03;  //set the TWI bus speed, frequency can not be larger than 400K, otherwise it does not work 5767 
  TWCR = 0x04;  //enable TWI bus
}

void read_5767(void)
{
  uchar k;
  Start();
  Wait();
  Writebyte(tea5767_add_r);
  Wait();
  for(k = 0; k< 5;k++)
  {
    Readbyte();
    Wait();
    rx[k] = TWDR;
  }
  Stop();
}

void write_5767(uint tx0,uint tx1,uchar tx2,uchar tx3,uchar tx4)
{
  Start();
  Wait();
  Writebyte(tea5767_add_w);
  Wait();
  Writebyte(tx0);
  Wait();
  Writebyte(tx1);                                      
  Wait();
  Writebyte(tx2);
  Wait();
  Writebyte(tx3);
  Wait();
  Writebyte(tx4);
  Wait();
  Stop();
}

void read_channel(void)
{
  e2prom_reads(1,receive,2);
  frequency_5767=(uint)receive[0]*256+receive[1];
  uart_printf("receive[0]=%d;receive[1]=%d;frequency=%d\r\n",receive[0],receive[1],frequency_5767);
  calculate_pll(frequency_5767);        
  write_5767(pll/256,pll%256,0x01,0x90,0x00);
}

void init_5767(void)
{
  calculate_pll(frequency_5767);
  write_5767(pll/256,pll%256,0x01,0x90,0x00);//Initialized: no search mode.
}

void init_sys(void)
{
   DDRD&=0x0f;
   PORTD|=0XF0;  //Enable pull-up resistor
   DDRB=0XFF;    //Set PORTB as output port data direction
   DDRA|=0xc0;   //Set PORTD as output port PORTA7 and PORTA6
   DDRC|=0X01;
}

uchar scan_press(void)
{
  i=PIND&0xf0;
  if(i==0xf0)
  {
    return 0;
  }
  else
  { 
    return 1;
  }
}

uchar key_scan(void)
{
  uchar value_key=0;
  if(scan_press()==1)
  {
    _delay_ms(8);
    switch (i)
    {
      case 0xf0:
        break;
                                        
      case 0x70:
        value_key=1;
        _delay_ms(30);
        break;
      case 0xb0:
        value_key=2;
        _delay_ms(30);
        break;
      case 0xd0:
        while((PIND&0XF0)==0Xd0)
        {
          long_key++;
          _delay_ms(2);
        }
        value_key=3;
        _delay_ms(30);
        break;
      case 0xe0:
        while((PIND&0XF0)==0XE0);
        _delay_ms(5);
        value_key=4;
        _delay_ms(30);
        break;
      default:
        value_key=0;
        _delay_ms(30);
        break;
    }
  }
  return value_key;
}

void key(void)
{
  scan_press();
  key_scan();
}

void min_control(void)
{
  if(frequen_temp1==-1)
  {
    frequen_temp1=9;
    frequen_temp2--;
  }
  if(frequen_temp2==-1)
  {
    frequen_temp2=9;
    frequen_temp3--;
  }        
  if(frequen_temp3==-1)
  {
    frequen_temp3=9;
    frequen_temp4--;
  }        
}

void add_control(void)
{
  if(frequen_temp1==10)
  {
    frequen_temp1=0;
    frequen_temp2++;                            
  }
  if(frequen_temp2==10)
  {
    frequen_temp2=0;
    frequen_temp3++;
  }        
  if(frequen_temp3==10)
  {
    frequen_temp3=0;
    frequen_temp4++;
  }        
}

void control_tea5767(void)
{
  uchar k=0;
  k=key_scan();
  if(k!=0)
  {
    switch (k)
    {
      case 1:
        if(flag_key==0)
        {       
          frequency_5767=frequency_5767+10;
          uart_printf("the frequency is %d\r\n",frequency_5767);            
          calculate_pll(frequency_5767);
          write_5767(pll/256,pll%256,0x01,0x90,0x00);
        }
        break;  //Increase the frequency
                                        
      case 2:
        if(flag_key==0)
        {       
          frequency_5767=frequency_5767-10;
          uart_printf("the frequency is %d\r\n",frequency_5767);
          calculate_pll(frequency_5767);
          write_5767(pll/256,pll%256,0x01,0x90,0x00);
        }
        break; //Decrease the frequency                    
      case 3:
        if(flag_key==0)
        {
          save_channel();
        }
        break;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 4:
        if(flag_key==0)
        {
          read_channel();
        }
        break;//Store channel
      default: 
          break;        
    }
  }
}

void dispaly_init()
{
  count_dis();
  display(1,2,"Designed by");
  display(2,2,"Liu Jiahang");
  _delay_ms(5000);
  write_com(0x01);
  _delay_ms(10);
        
  display(1,0,"Based On IIC-BUS");
  display(2,0,"TEA5767 FM TURNR");
  _delay_ms(5000);
  write_com(0x01);
  _delay_ms(10);
        
  display(1,3,"Starting...");
  display(2,1,"Please Waiting!");
  _delay_ms(3000);
  write_com(0x01);
  _delay_ms(10);
        
  display(1,10,"Stereo");
  display(2,1,"FM:");
  display(2,4,"  87.7  MHZ");
}

void autosearch_set()
{
  calculate_pll(frequency_5767);
  frequency_5767 = frequency_5767 + 20;
  uart_printf("autosearch_set,frequency %d\r\n",frequency_5767);
  WriteDataWord[0] = pll/256;
  WriteDataWord[1] = pll%256;
  WriteDataWord[0] |= 0x80;   //mute
  WriteDataWord[0] |= 0x40;   //Set to 1 for search start; other =0.
 
  //set the public data bit of BYTE3
  WriteDataWord[2] |= 0x00;
  WriteDataWord[2] |= 0x80;   //serch up
  WriteDataWord[2] |= 0x40;   //flaglevel                 
  WriteDataWord[2] &= 0xef;   //1110 1111   
  WriteDataWord[2] &= 0xf7 ;  //1111 0111   
   
  //set the public data bit of BYTE4
  WriteDataWord[3] |= 0x90;        
  WriteDataWord[3] &= 0x7f  ; //0111 1111   
   
  WriteDataWord[3] |= 0x08;   //mute       
  WriteDataWord[3] &= 0xfe; 

  //set the public data bit of BYTE4
  WriteDataWord[4] |= 0x00; 
  write_5767(WriteDataWord[0],WriteDataWord[1],WriteDataWord[2],WriteDataWord[3],WriteDataWord[4]);//Initialized: search mode.
}

void autosearch()
{
  uchar tempdatal, tempdatah, i;
  unsigned long temp_freq;
  unsigned long temp_data;
  autosearch_set();
  uart_printf("before read_5767 frequency = %d\r\n",frequency_5767);
  read_5767();
  tempdatal = rx[1];
  tempdatah = rx[0];
  tempdatah &= 0x3f;
  uart_printf("tempdatah %x;tempdatal = %x\r\n",tempdatah,tempdatal);

  pll = tempdatah*256+tempdatal;
  temp_data = (unsigned long)pll * 8192;
  temp_freq = (temp_data/1000+255)/10;
  
  uart_printf("pll = %d,temp_freq = %d;temp_data=UL%ld\r\n",pll,temp_freq,temp_data);
  while((rx[0] & 0x80) != 0x80)
  {
    read_5767();
    tempdatal = rx[1];
    tempdatah = rx[0];
    tempdatah &= 0x3f;
    uart_printf("tempdatah %x;tempdatal = %x\r\n",tempdatah,tempdatal);

    pll = tempdatah*256+tempdatal;
    temp_data = (unsigned long)pll * 8192;
    temp_freq = (temp_data/1000+255)/10;
    uart_printf("hold search!");
  }
  for(i=0;i<8;i++)
  {
    while(temp_freq <= frequency_tab_l[i])
    {
      autosearch_set();
      read_5767();
      tempdatal = rx[1];
      tempdatah = rx[0];
      tempdatah &= 0x3f;
      pll = tempdatah*256+tempdatal;
      temp_data = (unsigned long)pll * 8192;
      temp_freq = (temp_data/1000+255)/10;
      uart_printf("rx[0] %x;rx[1] = %x,temp_freq_l = %d\r\n",rx[0],rx[1],temp_freq);
      while((rx[0] & 0x80) != 0x80)
      {
       uart_printf("hold search!");
       read_5767();
       tempdatal = rx[1];
       tempdatah = rx[0];
       tempdatah &= 0x3f;
       uart_printf("tempdatah %x;tempdatal = %x\r\n",tempdatah,tempdatal);
     
       pll = tempdatah*256+tempdatal;
       temp_data = (unsigned long)pll * 8192;
       temp_freq = (temp_data/1000+255)/10;
       uart_printf("frequency_tab_l,hold search!");
      }
    }

    if((temp_freq - frequency_tab_l[i]) >= 2)
    {
      frequency_5767 = frequency_tab_l[i];
      PORTB |= (i << 4);
      uart_printf("mark-l:frequency_5767 %d,i=%d\r\n",frequency_5767,i);
      return;
    }
    else
    {
      autosearch_set();
    }
  }
  for(i=0;i<8;i++)
  {
    while(temp_freq <= frequency_tab_h[i])
    {
      autosearch_set();
      read_5767();
      tempdatal = rx[1];
      tempdatah = rx[0];
      tempdatah &= 0x3f;
      pll = tempdatah*256+tempdatal;
      temp_data = (unsigned long)pll * 8192;
      temp_freq = (temp_data/1000+255)/10;
      uart_printf("rx[0] %x;rx[1] = %x,temp_freq_h = %d\r\n",rx[0],rx[1],temp_freq);
      while((rx[0] & 0x80) != 0x80)
      {
        uart_printf("frequency_tab_l,hold search!");
        read_5767();
        tempdatal = rx[1];
        tempdatah = rx[0];
        tempdatah &= 0x3f;
        uart_printf("tempdatah %x;tempdatal = %x\r\n",tempdatah,tempdatal);
     
        pll = tempdatah*256+tempdatal;
        temp_data = (unsigned long)pll * 8192;
        temp_freq = (temp_data/1000+255)/10;
      }
    }

    if((temp_freq > frequency_tab_h[i]) > 2)
    {
      frequency_5767 = frequency_tab_h[i];
      PORTB |= (i << 4);
      uart_printf("mark-h:frequency_5767 %d,i=%d\r\n",frequency_5767,i);
      return;
    }
    else
    {
      autosearch_set();
    }
  }
}
/*********main function**********/
int main(void)
{
  init_sys();
  //init_1602();
  init_uart();
  init_twi();
  autosearch();
  init_5767();
  //dispaly_init();
  while(1)
  {
     key();
     control_tea5767();
  }
  return 0;
}
