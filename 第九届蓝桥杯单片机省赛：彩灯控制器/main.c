#include <STC15F2K60S2.H>
#include "iic.h"

sbit S7=P3^0;
sbit S6=P3^1;
sbit S5=P3^2;
sbit S4=P3^3;

unsigned char dat_rb2;																			//保存电位器Rb2的电压
unsigned char dat_led=0;
unsigned char stat=0;																				//彩灯当前状态
unsigned char mode =1;																			//彩灯工作模式
unsigned char mode_s6=1;																		//彩灯设置模式
unsigned char level=0;																			//彩灯亮度等级
unsigned int count_t800=0;																	//0.08秒间隔计数
unsigned int count=0;
unsigned int time_m[4];																			//四个模式流转间隔数组
unsigned char pwm_duty=20;																	//pwm脉宽
unsigned int t_pwm;
unsigned char f_open=0;																			//启动与停止
unsigned char f_set=0;																			//S6状态切换变量
unsigned char f_t800=0;																			//模式选择界面闪烁间隔
unsigned char code SMG_D[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,
														0xbf};	//10 -								

void LED_Working();

/*==================系统初始化函数====================*/
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
/*==================延时函数====================*/
void Delayms(unsigned int t)
{
	while(t--);
	while(t--);
}

void Delayus(unsigned int t)
{
	while(t--);
}

/*==================显示单位数码管函数====================*/
void DisplaySMG_Bit(unsigned char pos,unsigned char value)
{
	SelectHC573(6);
	P0=0x00;
	SelectHC573(6);
	P0=0x01<<pos;
	SelectHC573(7);
	P0=value;
}
/*==================显示全部数码管函数====================*/
void DisplaySMG_All(unsigned char value)
{
	SelectHC573(6);
	P0=0xff;
	SelectHC573(7);
	P0=value;
}
/*==================显示彩灯亮度等级函数====================*/
void DispaySMG_Level()
{
	DisplaySMG_Bit(0,0xff);
	DisplaySMG_Bit(1,0xff);
	DisplaySMG_Bit(2,0xff);
	DisplaySMG_Bit(3,0xff);
	DisplaySMG_Bit(4,0xff);
	DisplaySMG_Bit(5,0xff);
	DisplaySMG_Bit(6,SMG_D[10]);
	Delayus(100);
	DisplaySMG_Bit(7,SMG_D[level]);
	Delayus(100);
	DisplaySMG_All(0xff);
}
/*==================显示运行模式函数====================*/
void DisplaySMG_MOD()
{
	if(f_t800==0)
	{
		DisplaySMG_Bit(0,SMG_D[10]);
		Delayus(100);
		DisplaySMG_Bit(1,SMG_D[mode_s6]);
		Delayus(100);
		DisplaySMG_Bit(2,SMG_D[10]);
		Delayus(100);
	}
	else
	{
		DisplaySMG_Bit(0,SMG_D[10]);
		Delayus(100);
		DisplaySMG_Bit(1,0xff);
		DisplaySMG_Bit(2,SMG_D[10]);
		Delayus(100);
	}
	
	if(time_m[mode_s6-1]/1000!=0)
	{
		DisplaySMG_Bit(4,SMG_D[time_m[mode_s6-1]/1000]);
		Delayus(100);
	}
	DisplaySMG_Bit(5,SMG_D[(time_m[mode_s6-1]%1000)/100]);
	Delayus(100);
	DisplaySMG_Bit(6,SMG_D[0]);
	Delayus(100);
	DisplaySMG_Bit(7,SMG_D[0]);
	Delayus(100);
	DisplaySMG_All(0xff);
}
/*==================显示流转间隔设置函数====================*/
void DisplaySMG_Time()
{
	DisplaySMG_Bit(0,SMG_D[10]);
	Delayus(100);
	DisplaySMG_Bit(1,SMG_D[mode_s6]);
	Delayus(100);
	DisplaySMG_Bit(2,SMG_D[10]);
	Delayus(100);
	
	if(f_t800==0)
	{
		if(time_m[mode_s6-1]/1000!=0)
		{
			DisplaySMG_Bit(4,SMG_D[time_m[mode_s6-1]/1000]);
			Delayus(100);
		}
		DisplaySMG_Bit(5,SMG_D[(time_m[mode_s6-1]%1000)/100]);
		Delayus(100);
		DisplaySMG_Bit(6,SMG_D[0]);
		Delayus(100);
		DisplaySMG_Bit(7,SMG_D[0]);
		Delayus(100);
		DisplaySMG_All(0xff);
	}
	else
	{
		DisplaySMG_Bit(4,0xff);
		DisplaySMG_Bit(5,0xff);
		DisplaySMG_Bit(6,0xff);
		DisplaySMG_Bit(7,0xff);
	}
}
/*==================初始化定时器0====================*/
void InitTimer0()
{
	TMOD=0x01;
	TH0=(65535-1000)/256;
	TL0=(65535-1000)%256;
	ET0=1;
	EA=1;
	TR0=1;
}
/*==================定时器0中断函数====================*/
void ServiceTimer0() interrupt 1
{
	TH0=(65535-1000)/256;
	TL0=(65535-1000)%256;
	
	count_t800++;
	if(count_t800==800)
	{
		count_t800=0;
		f_t800= ~f_t800;
	}
	
	t_pwm++;
	if(t_pwm<=pwm_duty)
	{
		SelectHC573(4);
		P0=dat_led;
		SelectHC573(0);
	}
	else if(t_pwm<20)	//pwm脉宽处于设定值到20之间，熄灭
	{	
		SelectHC573(4);
		P0=0xff;
		SelectHC573(0);
	}
	else		//t_pwm大于20后重置
	{
		SelectHC573(4);
		P0=dat_led;
		SelectHC573(0);
		t_pwm=0;
		LED_Working();
	}
	
	count++;
	if(count==time_m[mode-1])	
	{
		count=0;
		if(f_open==1)
			stat++;
		if(stat==25)
			stat=0;
	}
	
}
/*==================8-24C02字节写====================*/
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
/*==================8-24C02字节读====================*/
unsigned char Read_24C02(unsigned char addr)
{
	unsigned char tmp;
	IIC_Start();
	IIC_SendByte(0xa0);
	IIC_WaitAck();
	IIC_SendByte(addr);
	IIC_WaitAck();
	
	IIC_Start();
	IIC_SendByte(0xa1);
	IIC_WaitAck();
	tmp=IIC_RecByte();
	IIC_SendAck(0);
	IIC_Stop();
	return tmp;
}
/*==================PCF8591数据采集====================*/
 void Read_Rb2()
 {
	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(0x03);
	IIC_WaitAck();
	IIC_Stop();
	 
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	dat_rb2=IIC_RecByte();
	IIC_SendAck(0);
	IIC_Stop();
 }
/*==================彩灯亮度控制====================*/
void Level_Change()
{
	if(dat_rb2<60)
	{
		pwm_duty=5;
		level=1;
	}
	else if(dat_rb2<120)
	{
		pwm_duty=10;
		level=2;
	}
	else if(dat_rb2<180)
	{
		pwm_duty=15;
		level=3;
	}
	else
	{
		pwm_duty=20;
		level=4;
	}
}
/*==================彩灯四种模式变化====================*/
void LED_Working()
{
	switch(stat)
	{
		case 0:
			dat_led=0xff;
		break;
		//模式1彩灯变化形态
		case 1:
			dat_led=0xfe;
		break;
		case 2:
			dat_led=0xfc;
		break;
		case 3:
			dat_led=0xf8;
		break;
		case 4:
			dat_led=0xf0;
		break;
		case 5:
			dat_led=0xe0;
		break;
		case 6:
			dat_led=0xc0;
		break;
		case 7:
			dat_led=0x80;
		break;
		case 8:
			dat_led=0x00;
		break;
		//模式2彩灯变化形态
		case 9:
			dat_led=0x7f;
		break;
		case 10:
			dat_led=0x3f;
		break;
		case 11:
			dat_led=0x1f;
		break;
		case 12:
			dat_led=0x0f;
		break;
		case 13:
			dat_led=0x07;
		break;
		case 14:
			dat_led=0x03;
		break;
		case 15:
			dat_led=0x01;
		break;
		case 16:
			dat_led=0x00;
		break;
		//模式3彩灯变化形态
		case 17:
			dat_led=0x7e;
		break;
		case 18:
			dat_led=0xbd;
		break;
		case 19:
			dat_led=0xdb;
		break;
		case 20:
			dat_led=0xe7;
		break;
		//模式4彩灯变化形态
		case 21:
			dat_led=0xe7;
		break;
		case 22:
			dat_led=0xdb;
		break;
		case 23:
			dat_led=0xbd;
		break;
		case 24:
			dat_led=0x7e;
		break;
	}
	if(stat==0)
	{
		mode=1;
	}
	else if(stat==9)
	{
		mode=2;
	}
	else if(stat==17)
	{
		mode=3;
	}
	else if(stat==21)
	{
		mode=4;
	}
}
/*==================按键扫描与处理====================*/
void Scan_Key()
{
	if(S7==0)
	{
		Delayus(100);
		if(S7==0)
		{
			if(f_open==0)
			{
				f_open=1;
			}
			else
			{
				f_open=0;
				f_set=0;
				stat=0;
				mode=1;
			}
			while(S7==0);
		}
	}
	if(S6==0)
	{
		Delayus(100);
		if(S6==0)
		{
			f_set++;									//模式加一
			while(S6==0)							//等待按键松开时数码管进行相应的显示	
			{
				if(f_set==1)
				{
					DisplaySMG_MOD();			//显示运行模式
				}
				else if(f_set==2)
				{
					DisplaySMG_Time();		//显示流转间隔设置
				}
			}
		}
	}
	if(S5==0)
	{
		Delayus(100);
		if(S5==0)
		{
			if(f_set==1)							//当处于运行模式设置界面
			{
				mode_s6+=1;							//运行模式加一
				if(mode_s6>4)						//模式边界处理
				{
					mode_s6=4;
				}
				while(S5==0)						//等待按键松开时数码管进行相应的显示	
				{
					DisplaySMG_MOD();
				}
			}
			else if(f_set==2)					//当处于流转间隔设置界面
			{
				time_m[mode_s6-1]+=100;	//流转间隔时间增加100ms
				if(time_m[mode_s6-1]>1200)
				{
					time_m[mode_s6-1]=1200;
				}
				while(S5==0)
				{
					DisplaySMG_Time();
				}
			}
		}
	}
	if(S4==0)
	{
		Delayus(100);
		if(S4==0)
		{
			if(f_set==1)
			{
				mode_s6-=1;
				if(mode_s6<1)
				{
					mode_s6=1;
				}
				while(S4==0)
				DisplaySMG_MOD();
			}
			else if(f_set==2)
			{
				time_m[mode_s6-1]-=100;	
				if(time_m[mode_s6-1]<400)
				{
					time_m[mode_s6-1]=400;
				}
				while(S4==0)
				DisplaySMG_Time();
			}
			else if(f_set==0)
			{
				DispaySMG_Level();
				LED_Working();
			}
		}
	}
}
/*==================运行模式和流转间隔参数保存====================*/
void Save_Config()
{
	switch(mode_s6)
	{
		case 1:
			Write_24C02(0x01,time_m[0]/10);
		break;
		case 2:
			Write_24C02(0x02,time_m[1]/10);
		break;
		case 3:
			Write_24C02(0x03,time_m[2]/10);
		break;
		case 4:
			Write_24C02(0x04,time_m[3]/10);
		break;
	}
	Delayus(1000);
	DisplaySMG_All(0xff);
	mode_s6=1;
	f_set=0;
}
/*==================系统初始化====================*/
void Init_System()
{
	SelectHC573(5);
	P0=0x00;
	SelectHC573(4);
	P0=0xff;
	SelectHC573(0);
	
	time_m[0]=Read_24C02(0x01)*10;
	time_m[1]=Read_24C02(0x02)*10;
	time_m[2]=Read_24C02(0x03)*10;
	time_m[3]=Read_24C02(0x04)*10;
	
	InitTimer0();
}
/*==================主函数====================*/
void main()
{
	Init_System();
	while(1)
	{
		Scan_Key();
		Read_Rb2();
		Level_Change();
		switch(f_set)
		{
			case 1:
				DisplaySMG_MOD();
			break;
			case 2:
				DisplaySMG_Time();
			break;
			case 3:
				Save_Config();
			break;
		}
	}
}
