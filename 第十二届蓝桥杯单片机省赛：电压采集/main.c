#include <STC15F2K60S2.H>
#include "iic.h"

unsigned int count_f=0;		//频率计数
unsigned int count_t=0;		//定时器计数
unsigned int dat_f=0;     //频率
unsigned int dat_t=0;			//周期
unsigned int dat_rb1=0;		//光敏电阻
unsigned int dat_rb2=0;		//rb2电阻
unsigned int dat_vb1=0;		//光敏电压
unsigned int dat_vb2=0;		//rb2电压
unsigned char channel=1;  //电压通道 
unsigned char s4_mode=1;	//界面切换设置
unsigned char s7_mode=1;	//S7按键功能：1短按，2长按
bit f_led=1;							//LED功能使能标记
bit f_s7=0;								//S7长按标记
unsigned int count_s7=0;	//S7长按计数
unsigned char save_vb2=0;
unsigned char save_f=0;

sbit S4=P3^3;
sbit S5=P3^2;
sbit S6=P3^1;
sbit S7=P3^0;

sbit L1=P0^0;
sbit L2=P0^1;
sbit L3=P0^2;
sbit L4=P0^3;
sbit L5=P0^4;

unsigned char code SMG_D[10]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
unsigned char code SMG_D2[10]={0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10};
void Init_Timer();
void DigDisplay();
															
/*=====================系统初始化=====================*/
void SelectHC573(unsigned char channel)
{
	switch(channel)
	{
		case 4:
			P2=(P2&0x1f)|0x80;
		break;
		case 5:
			P2=(P2&0x1f)|0xa0;
		break;
		case 6:
			P2=(P2&0x1f)|0xc0;
		break;
		case 7:
			P2=(P2&0x1f)|0xe0;
		break;
		case 0:
			P2=(P2&0x1f)|0x00;
		break;
	}
}

void Init_System()
{
	SelectHC573(5);
	P0=0x00;
	SelectHC573(4);
	P0=0xff;
	SelectHC573(0);
	Init_Timer();
}
/*=====================各种延时函数=====================*/
void Delayus(unsigned int t)
{
	while(t--);
}
/*=====================数码管基础函数=====================*/
void DisplaySMG_Bit(unsigned char pos,unsigned char value)
{
	SelectHC573(6);
	P0=0x00;
	SelectHC573(6);
	P0=0x01<<pos;
	SelectHC573(7);
	P0=value;
}

void DisplaySMG_All(unsigned char value)
{
	SelectHC573(6);
	P0=0xff;
	SelectHC573(7);
	P0=value;
}
/*=====================定时器=====================*/
void Init_Timer()
{
	TH0=0xff;
	TL0=0xff;
	
	TH1=(65536-50000)/256;
	TL1=(65536-50000)%256;
	
	TMOD=0x06;
	ET0=1;
	ET1=1;
	EA=1;
	TR0=1;
	TR1=1;
}
/*=====================定时器中断服务=====================*/
void Service_Timer0() interrupt 1
{
	count_f++;
}

void Service_Timer1() interrupt 3
{
	TH1=(65536-50000)/256;
	TL1=(65536-50000)%256;
	count_t++;
	if(count_t==20)
	{
		dat_f=count_f;
		dat_t=10000/dat_f;
		count_f=0;
		count_t=0;
	}
	if(f_s7)
	{
		count_s7++;
		if(count_s7==20)
		{
			count_s7=20;
			s7_mode=2;
		}
	}
}
/*=====================可调电阻电压输出=====================*/
void Read_rb2()
{
	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(0x03);
	IIC_WaitAck();
	IIC_Stop();
	
	DigDisplay();
	
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	dat_rb2=IIC_RecByte();
	IIC_SendAck(1);
	IIC_Stop();
	
	dat_vb2=dat_rb2*1.96+0.2;
	DigDisplay();
}
/*=====================光敏电阻电压输出=====================*/
void Read_rb1()
{
	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(0x01);
	IIC_WaitAck();
	IIC_Stop();
	
	DigDisplay();
	
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	dat_rb1=IIC_RecByte();
	IIC_SendAck(1);
	IIC_Stop();
	
	dat_vb1=dat_rb1*1.96+0.2;
	DigDisplay();
}
/*=====================24C02相关函数=====================*/
void Write_24C02(unsigned char addr,unsigned char dat)
{
	IIC_Start();
	IIC_SendByte(0xa0);
	IIC_WaitAck();
	IIC_SendByte(addr);
	IIC_WaitAck();
	IIC_SendByte(dat);
	IIC_WaitAck();
	IIC_Stop();
}

unsigned char Read_24C02(unsigned char addr)
{
	unsigned char temp;
	IIC_Start();
	IIC_SendByte(0xa0);
	IIC_WaitAck();
	IIC_SendByte(addr);
	IIC_WaitAck();
	
	IIC_Start();
	IIC_SendByte(0xa1);
	IIC_WaitAck();
	temp=IIC_RecByte();
	IIC_SendAck(1);
	IIC_Stop();
	return temp;
}
/*=====================频率显示界面=====================*/
void DisplaySMG_F()
{
	DisplaySMG_Bit(0,0x8e);
	Delayus(100);
	if(dat_f>999999)
	{
		DisplaySMG_Bit(1,SMG_D[dat_f/1000000]);
		Delayus(100);
	}
	if(dat_f>99999)
	{
		DisplaySMG_Bit(2,SMG_D[(dat_f/100000)%10]);
		Delayus(100);
	}
	if(dat_f>9999)
	{
		DisplaySMG_Bit(3,SMG_D[(dat_f/10000)%10]);
		Delayus(100);
	}
	if(dat_f>999)
	{
		DisplaySMG_Bit(4,SMG_D[(dat_f/1000)%10]);
		Delayus(100);
	}
	if(dat_f>99)
	{
		DisplaySMG_Bit(5,SMG_D[(dat_f/100)%10]);
		Delayus(100);
	}
	if(dat_f>9)
	{
		DisplaySMG_Bit(6,SMG_D[(dat_f/10)%10]);
		Delayus(100);
	}
	DisplaySMG_Bit(7,SMG_D[dat_f%10]);
	Delayus(100);
	DisplaySMG_All(0xff);
}
/*=====================周期显示界面=====================*/
void DisplaySMG_T()
{
	DisplaySMG_Bit(0,0xc8);
	Delayus(100);
	if(dat_t>999999)
	{
		DisplaySMG_Bit(1,SMG_D[dat_t/1000000]);
		Delayus(100);
	}
	if(dat_t>99999)
	{
		DisplaySMG_Bit(2,SMG_D[(dat_t/100000)%10]);
		Delayus(100);
	}
	if(dat_t>9999)
	{
		DisplaySMG_Bit(3,SMG_D[(dat_t/10000)%10]);
		Delayus(100);
	}
	if(dat_t>999)
	{
		DisplaySMG_Bit(4,SMG_D[(dat_t/1000)%10]);
		Delayus(100);
	}
	if(dat_t>99)
	{
		DisplaySMG_Bit(5,SMG_D[(dat_t/100)%10]);
		Delayus(100);
	}
	if(dat_t>9)
	{
		DisplaySMG_Bit(6,SMG_D[(dat_t/10)%10]);
		Delayus(100);
	}
	DisplaySMG_Bit(7,SMG_D[dat_t%10]);
	Delayus(100);
	DisplaySMG_All(0xff);
}
/*=====================电压显示界面=====================*/
void DisplaySMG_V()
{
	DisplaySMG_Bit(0,0xc1);
	Delayus(100);
	DisplaySMG_Bit(1,0xbf);
	Delayus(100);
	DisplaySMG_Bit(2,SMG_D[channel]);
	Delayus(100);
	if(channel==1)
	{
		DisplaySMG_Bit(5,SMG_D2[dat_vb1/100]);
		Delayus(100);
		DisplaySMG_Bit(6,SMG_D[(dat_vb1/10)%10]);
		Delayus(100);
		DisplaySMG_Bit(7,SMG_D[dat_vb1%10]);
		Delayus(100);
	}
	else if(channel==3)
	{
		DisplaySMG_Bit(5,SMG_D2[dat_vb2/100]);
		Delayus(100);
		DisplaySMG_Bit(6,SMG_D[(dat_vb2/10)%10]);
		Delayus(100);
		DisplaySMG_Bit(7,SMG_D[dat_vb2%10]);
		Delayus(100);
	}
	DisplaySMG_All(0xff);
}
/*=====================数码管显示=====================*/
void DigDisplay()
{
	switch(s4_mode)
	{
		case 1:
			DisplaySMG_F();
		break;
		case 2:
			DisplaySMG_T();
		break;
		case 3:
			DisplaySMG_V();
		break;
	}
}
/*=====================LED工作=====================*/
void LedWorking()
{
	save_vb2=Read_24C02(0x01);	//3
	save_f=Read_24C02(0x02);		//1
	if(f_led)
	{
		if(save_vb2<dat_vb2)
		{
			SelectHC573(4);
			L1=0;
			SelectHC573(0);
		}
		else
		{
			SelectHC573(4);
			L1=1;
			SelectHC573(0);
		}
		if(save_f<dat_f)
		{
			SelectHC573(4);
			L2=0;
			SelectHC573(0);
		}
		else
		{
			SelectHC573(4);
			L2=1;
			SelectHC573(0);
		}
		if(s4_mode==1)
		{
			SelectHC573(4);
			L3=0;
			SelectHC573(0);
		}
		else L3=1;
		if(s4_mode==2)
		{
			SelectHC573(4);
			L4=0;
			SelectHC573(0);
		}
		else L4=1;
		if(s4_mode==3)
		{
			SelectHC573(4);
			L5=0;
			SelectHC573(0);
		}
		else L5=1;
	}
	else
	{
		SelectHC573(4);
		P0=0xff;
		SelectHC573(0);
	}
}
/*=====================键盘扫描=====================*/
void ScanKey()
{
	if(S4==0)
	{
		while(S4==0)
		{
			DigDisplay();
			LedWorking();
		}
		s4_mode++;
		if(s4_mode>3)
		s4_mode=1;
		
	}
	else if(S5==0)
	{
		while(S5==0)
		{
			DigDisplay();
			LedWorking();
		}
		channel=channel+2;
		if(channel>3)
		channel=1;
	}
	else if(S6==0)
	{
		while(S6==0)
		{
			DigDisplay();
			LedWorking();
		}
		Write_24C02(0x01,dat_vb2);
	}
	else if(S7==0)
	{
		while(S7==0)
		{
			DigDisplay();
			LedWorking();
			f_s7=1;
		}
		f_s7=0;
		count_s7=0;
		if(s7_mode==1)
		{
			Write_24C02(0x02,dat_f);
		}
		else if(s7_mode==2)
		{
			f_led=~f_led;
			s7_mode=1;
		}
	}
}
/*=====================主函数=====================*/
void main()
{
	Init_System();
	while(1)
	{
		Read_rb2();
		Read_rb1();
		LedWorking();
		DigDisplay();
		ScanKey();
	}
}