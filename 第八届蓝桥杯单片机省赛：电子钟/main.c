# include "reg52.h"
# include "ds1302.h"
# include "onewire.h"

typedef unsigned char uchar;
typedef unsigned int uint;

sbit S7 = P3^0;
sbit S6 = P3^1;
sbit S5 = P3^2;
sbit S4 = P3^3;

sbit L1 = P0^0;

uchar duanma[18] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x80,0xc6,0xc0,0x86,0x8e,0xbf,0x7f};
uchar Write_addr[3] = {0x80,0x82,0x84};	   //DS1302时钟每一位写入的地址
uchar Read_addr[3] = {0x81,0x83,0x85};	   //DS1302时钟每一位读取的地址
uchar Timer[3] = {87,89,35};		       //用来存储读取的时间，初始化为23时59分50秒
uchar Clock[3] = {0,0,0};		           //用来存储闹钟的时间，初始化为00时00分00秒
uint temp = 0;							   //用来存储读取的温度
uint count = 0;							   //定时器的计数变量
bit smg_f = 1;						       //控制数码管闪烁的标志变量
uint led_c = 0;							   //控制LED的计数变量变量
bit led_f = 0;							   //控制LED闪烁的标志变量
bit led_k = 0;							   //控制led的开关变量,0为关
uchar k7 = 0;							   //记录S7的当前状态，0为时钟显示，1为设置时钟的时
uchar k6 = 0;							   //记录S6的当前状态，0为S6未按下，1为设置闹钟的时

void LEDRunning ();						   //对LED显示函数进行声明

//=======================锁存器的控制===========================
void SelectHC573 (uchar n)
{
	switch (n)
	{
		case 4:
			P2 = (P2 & 0x1f) | 0x80;break;
		case 5:
			P2 = (P2 & 0x1f) | 0xa0;break;
		case 6:
			P2 = (P2 & 0x1f) | 0xc0;break;
		case 7:
			P2 = (P2 & 0x1f) | 0xe0;break;
		case 0:
			P2 = (P2 & 0x1f) | 0x00;break;

	}
}
//==============================================================

/*========================普通延时函数==========================
void Delay (uint t)
{
	while (t--);
}
==============================================================*/

//========================BCD转十进制===========================
uchar BCDtoTEN (uchar dat)
{
	dat = (dat /16) * 10 + (dat % 16);
	return dat;	
}
//==============================================================

//========================Timer[i]BCD的加减 ====================
uchar Timeradd(uchar dat)
{
	dat = dat + 1;
	switch(dat)
	{
		case 10:
			dat = 16;break;			 //BCD的10
		case 26:
			dat = 32;break;			 //BCD的20
		case 42:
			dat = 48;break;			 //BCD的30
		case 58:
			dat = 64;break;			 //BCD的40
		case 74:
			dat = 80;break;			 //BCD的50
		case 90:
			dat = 96;break;			 //BCD的60
	}
	return dat;
}

uchar Timerminus(uchar dat)
{
	dat = dat - 1;
	switch(dat)
	{
		case 79:
			dat = 73;break;	   
		case 63:
			dat = 57;break;
		case 47:
			dat = 41;break;
		case 31:
			dat = 25;break;
		case 15:
			dat = 9;break;
		case -1:
			dat = 89;break;		
	}
	return dat;
}
//==============================================================

//========================系统初始化============================
void InitSystem ()
{
	SelectHC573(4);
	P0 = 0xff;
	SelectHC573(5);
	P0 = 0x00;
	SelectHC573(0);	
}
//==============================================================

//=====================定时器T0 - 10ms =========================
void InitTime0 ()
{
	TMOD = 0x01;

	TH0 = (65535 - 10000) / 256;
	TL0 = (65535 - 10000) % 256;

	TR0 = 1;
	ET0 = 1;
	EA = 1;
}

void ServiceT0 () interrupt 1
{
	TH0 = (65535 - 10000) / 256;
	TL0 = (65535 - 10000) % 256;
	
	count++;
	if (count == 100)
	{
		smg_f = ~smg_f;
		count = 0;		
	}
	
	if (led_k == 1)
	{
		led_c ++;
		if (led_c % 20 == 0)
		{
			led_f = ~led_f;
			if (led_c >= 500)
			{
				led_k = 0;
				led_c = 0;
				led_f = 0;
			}	
		}	
	}	
}
//==============================================================

//===================DS1302实时时钟的配置与读取=================
void SetDS1302 ()
{
	uchar i;

	Write_Ds1302_Byte(0x8e,0x00);
	for (i = 0;i<=2;i++)
	{
		Write_Ds1302_Byte(Write_addr[i],Timer[i]);
	}
	Write_Ds1302_Byte(0x8e,0x80);
}

void ReadDS1302 ()
{
	uchar i;

	for (i = 0;i<=2;i++)
	{
		Timer[i] = Read_Ds1302_Byte(Read_addr[i]);
	}	
}
//==============================================================

//=====================DS18B20温度读取==========================
void Read_Temp ()
{
	uchar LSB;
	uchar MSB;

	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x44);	
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0xbe);
	LSB = Read_DS18B20();
	MSB = Read_DS18B20();

	temp = (MSB << 8) | LSB;

	if ((temp & 0xf800) == 0x0000)
	{
		temp = temp >> 4;
	}
}
//==============================================================

//====================数码管相关显示函数========================
void Delay_SMG(uint t)
{
	while (t--);
}

void ShowSMG_Bit (uchar pos,uchar dat)
{
	SelectHC573(7);
	P0 = 0xff;
	SelectHC573(6);
	P0 = 0x01 << pos - 1;
	SelectHC573(7);
	P0 = dat;
	SelectHC573(0);	
}

void AllSMG (uchar dat)
{
	SelectHC573(6);
	P0 = 0xff;
	SelectHC573(7);
	P0 = dat;
	SelectHC573(0);	
}

void ShowSMG_Time ()
{
	if (k6 == 0)
	{
		if (((k7 == 1) & (smg_f == 1)) | (k7 == 0) | (k7 == 2) | (k7 == 3))
		{
			ShowSMG_Bit(1,duanma[Timer[2] / 16]);
			Delay_SMG(500);
			ShowSMG_Bit(2,duanma[Timer[2] % 16]);
			Delay_SMG(500);
		}
		ShowSMG_Bit(3,duanma[16]);
		Delay_SMG(500);
		if (((k7 == 2) & (smg_f == 1)) | (k7 == 0) | (k7 == 1) | (k7 == 3))
		{
			ShowSMG_Bit(4,duanma[Timer[1] / 16]);
			Delay_SMG(500);
			ShowSMG_Bit(5,duanma[Timer[1] % 16]);
			Delay_SMG(500);
		}
		ShowSMG_Bit(6,duanma[16]);
		Delay_SMG(500);
		if (((k7 == 3) & (smg_f == 1)) | (k7 == 0) | (k7 == 2) | (k7 == 1))
		{
			ShowSMG_Bit(7,duanma[Timer[0] / 16]);
			Delay_SMG(500);
			ShowSMG_Bit(8,duanma[Timer[0] % 16]);
			Delay_SMG(500);
		}
		AllSMG(0xff);	
	}
	else if (k6 != 0)
	{
		if (((k6 == 1) & (smg_f == 1)) | (k6 == 0) | (k6 == 2) | (k6 == 3))
		{
			ShowSMG_Bit(1,duanma[Clock[2] / 10]);
			Delay_SMG(500);
			ShowSMG_Bit(2,duanma[Clock[2] % 10]);
			Delay_SMG(500);
		}
		ShowSMG_Bit(3,duanma[16]);
		Delay_SMG(500);
		if (((k6 == 2) & (smg_f == 1)) | (k6 == 0) | (k6 == 1) | (k6 == 3))
		{
			ShowSMG_Bit(4,duanma[Clock[1] / 10]);
			Delay_SMG(500);
			ShowSMG_Bit(5,duanma[Clock[1] % 10]);
			Delay_SMG(500);
		}
		ShowSMG_Bit(6,duanma[16]);
		Delay_SMG(500);
		if (((k6 == 3) & (smg_f == 1)) | (k6 == 0) | (k6 == 2) | (k6 == 1))
		{
			ShowSMG_Bit(7,duanma[Clock[0] / 10]);
			Delay_SMG(500);
			ShowSMG_Bit(8,duanma[Clock[0] % 10]);
			Delay_SMG(500);
		}
		AllSMG(0xff);	
	}
}

void ShowSMG_Temp ()
{
	ShowSMG_Bit(6,duanma[temp / 10]);
	Delay_SMG(500);
	ShowSMG_Bit(7,duanma[temp % 10]);
	Delay_SMG(500);
	ShowSMG_Bit(8,duanma[12]);
	Delay_SMG(500);	
	AllSMG(0xff);
}

//==============================================================

//======================浏览按键================================
void Delay_Key (uchar t)
{
	while (t--);
}

void ScanKey ()
{
	if (S7 == 0)
	{
		Delay_Key(100);
		if (S7 == 0)
		{
			while (S7 == 0)
			{
				ShowSMG_Time ();
				LEDRunning ();
				ReadDS1302 ();	
			}
			if (led_k == 1)
			{
				led_k = 0;
				led_c = 0;
				led_f = 0;
			}
			k7++;
			if (k7 >= 4)
			{
				k7 = 0;
			}
		}	
	}
	if (S6 == 0)
	{
		Delay_Key(100);
		if (S6 == 0)
		{
			while (S6 == 0)
			{
				ShowSMG_Time ();
				LEDRunning ();
				ReadDS1302 ();	
			}
			if (led_k == 1)
			{
				led_k = 0;
				led_c = 0;
				led_f = 0;
			}
			k6++;
			if (k6 >= 4)
			{
				k6 = 0;
			} 
		}	
	}
	if (S5 == 0)
	{
		Delay_Key(100);
		if (S5 == 0)
		{
			while (S5 == 0)
			{
				ShowSMG_Time ();
				LEDRunning ();
				ReadDS1302 ();	
			}
			if (led_k == 1)
			{
				led_k = 0;
				led_c = 0;
				led_f = 0;
			}
			if (k7 == 1)
			{
				Timer[2] = Timeradd(Timer[2]);			
				if (Timer[2] >= 96)
				{
					Timer[2] = 0;
				}
			}
			else if (k7 == 2)
			{	
				Timer[1] = Timeradd(Timer[1]);
				if (Timer[1] >= 96)
				{
					Timer[1] = 0;
				}	
			}
			else if (k7 == 3)
			{	
				Timer[0
				] = Timeradd(Timer[0]);
				if (Timer[0] >= 96)
				{
					Timer[0] = 0;
				}	
			}
			if (k6 == 1)		 
			{	
				Clock[2] = Clock[2] + 1;
				if (Clock[2] >= 24)
				{
					Clock[2] = 0;
				}	
			}
			if (k6 == 2)
			{	
				Clock[1] = Clock[1] + 1;
				if (Clock[1] >= 60)
				{
					Clock[1] = 0;
				}	
			}
			if (k6 == 3)
			{	
				Clock[0] = Clock[0] + 1;
				if (Clock[0] >= 60)
				{
					Clock[0] = 0;
				}	
			}
			SetDS1302 ();
		}	
	}
	if (S4 == 0)
	{
		Delay_Key(100);
		if (S4 == 0)
		{
			while (S4 == 0)
			{
				if ((k6 == 0) & (k7 == 0))
				{
					Read_Temp ();
					ShowSMG_Temp ();	
				}
				else 
				{
					ShowSMG_Time ();
					LEDRunning ();	
				}
				ReadDS1302 ();	
			}
			if (led_k == 1)
			{
				led_k = 0;
				led_c = 0;
				led_f = 0;
			}
			if (k7 == 1)
			{
				Timer[2] = Timerminus(Timer[2]);			
				if (Timer[2] == 89)
				{
					Timer[2] = 35;
				}
			}
			else if (k7 == 2)
			{	
				Timer[1] = Timerminus(Timer[1]);	
			}
			else if (k7 == 3)
			{	
				Timer[0] = Timerminus(Timer[0]);	
			}

			if (k6 == 1)		 
			{	
				Clock[2] = Clock[2] - 1;
				if (Clock[2] == -1)
				{
					Clock[2] = 23;
				}	
			}
			else if (k6 == 2)
			{	
				Clock[1] = Clock[1] - 1;
				if (Clock[1] == -1)
				{
					Clock[1] = 59;
				}	
			}
			else if (k6 == 3)
			{	
				Clock[0] = Clock[0] - 1;
				if (Clock[0] == -1)
				{
					Clock[0] = 59;
				}	
			}
			SetDS1302 ();
		}	
	}	
}
//==============================================================

//======================LED显示函数=============================
void LEDRunning ()
{
	if ((BCDtoTEN(Timer[0]) == Clock[0]) & (BCDtoTEN(Timer[1]) == Clock[1]) & (BCDtoTEN(Timer[2]) == Clock[2]))
	{
		led_k = 1;		
	}
	if (led_f == 1)
	{
		SelectHC573(4);
		L1 = 0;	
		SelectHC573(0);
	}
	else if (led_f == 0)
	{
		SelectHC573(4);
		L1 = 1;	
		SelectHC573(0);
	}	
}
//==============================================================

//=========================主函数===============================
void main (void)
{
	InitSystem ();
	SetDS1302 ();
	InitTime0 ();
	while (1)
	{	
		ReadDS1302 ();
		ScanKey ();
		ShowSMG_Time ();
		LEDRunning ();	
	}
}
//==============================================================
