	#include <STC15F2K60S2.H>
	#include <stdio.h>
	#include <stdarg.h>
	#include <intrins.h>

	/*总线矩阵检测*/
	sbit SA1 = P3^5;
	sbit SA2 = P3^6;
	sbit SA3 = P3^7;
	sbit SA4 = P4^1;

	sbit SB1 = P4^2;
	sbit SB2 = P4^3;
	sbit SB3 = P4^4;
	sbit SB4 = P2^0;

	sbit SC1 = P2^4;
	sbit SC2 = P2^5;
	sbit SC3 = P2^6;
	sbit SC4 = P2^7;

	sbit SD1 = P4^5;
	sbit SD2 = P4^6;
	sbit SD3 = P0^0;
	sbit SD4 = P0^1;

	sbit SE1 = P0^2;
	sbit SE2 = P0^3;
	sbit SE3 = P0^4;

	sbit A0  = P5^4;
	sbit A1  = P1^7;
	sbit A2  = P5^5;

	sbit B0  = P3^2;
	sbit B1  = P4^0;
	sbit B2  = P3^3;

	sbit C0  = P2^2;
	sbit C1  = P2^1;
	sbit C2  = P2^3;

	sbit D0  = P0^6;
	sbit D1  = P0^5;
	sbit D2  = P0^7;

	sbit E0  = P1^2;
	sbit E1  = P4^7;
	sbit E2  = P1^3;

	#define NONE_PARITY     0       //无校验
	#define ODD_PARITY      1       //奇校验
	#define EVEN_PARITY     2       //偶校验
	#define MARK_PARITY     3       //标记校验
	#define SPACE_PARITY    4       //空白校验
	
	#define S1_S0 0x40              //P_SW1.6
	#define S1_S1 0x80              //P_SW1.7
	#define S2_S0 0x01              //P_SW2.0
	#define S2RI  0x01              //S2CON.0
	#define S2TI  0x02              //S2CON.1
	#define S2RB8 0x04              //S2CON.2
	#define S2TB8 0x08              //S2CON.3

	#define	BOOT 		0x00		//空闲
	#define STEP 		0x01		//单步检测
	#define ROLL 		0x02		//滚动检测
	#define DEBUG		0x03		//开关debug
	#define HELP		0x04		//命令帮助
	#define REBOOT  0x05		//重启
	#define AUTO  	0x06		//自动检测

	#define FOSC 11059200L          //系统频率

	#define UART1_IO					0				//定义串口1IO口复用选择
	#define UART1_PARITYBIT NONE_PARITY   	//定义校验位
	#define UART1_BODE		9600				//串口波特率
	
	#define UART2_IO					0				//定义串口2IO口复用选择
	#define UART2_PARITYBIT NONE_PARITY   	//定义校验位
	#define UART2_BODE		9600				//串口波特率

	#define REPORT	UART2_SendString
  #define PRINTF 	UART1_SendData
  
	static unsigned char err  = 0;
	static unsigned char err_last  = 0;
	static bit f_debug = 0;
	static bit tx1_busy = 0;
	static bit tx2_busy = 0;
	int line = -1;
	int line_last = -1;
	int bus[4];		
	int pos[5];
	unsigned char cmd = ROLL;
	unsigned char cmd_last = ROLL;


	//延时函数us
	void delay_us(unsigned int t)
	{
		while(t)
		{
			_nop_();
			_nop_();
			_nop_();
			t--;
		}
	}
	//延时函数ms
	void delay_ms(unsigned int t)
	{
		unsigned char i, j;
		while(t)
		{
			_nop_();
			_nop_();
			_nop_();
			i = 11;
			j = 190;
			do
			{
				while (--j);
			} while (--i);
			t--;
		}
	}

	//串口1初始化
	void Init_uart1(unsigned int baud){
		
		#if (UART1_IO == 0)
		{
			ACC = P_SW1;
			ACC &= ~(S1_S0 | S1_S1);    //S1_S0=0 S1_S1=0
			P_SW1 = ACC;                //(P3.0/RxD, P3.1/TxD)
    }
		#elif (UART1_IO == 1)
		{
			ACC = P_SW1;
			ACC &= ~(S1_S0 | S1_S1);    //S1_S0=1 S1_S1=0
			ACC |= S1_S0;               //(P3.6/RxD_2, P3.7/TxD_2)
			P_SW1 = ACC;  
		}
		#elif (UART1_IO == 2)
		{
			ACC = P_SW1;
			ACC &= ~(S1_S0 | S1_S1);    //S1_S0=0 S1_S1=1
			ACC |= S1_S1;               //(P1.6/RxD_3, P1.7/TxD_3)
			P_SW1 = ACC;  
		}
		#endif
		
		#if (UART1_PARITYBIT == NONE_PARITY)
				SCON = 0x50;                //8位可变波特率
		#elif (UART1_PARITYBIT == ODD_PARITY) || (UART1_PARITYBIT == EVEN_PARITY) || (UART1_PARITYBIT == MARK_PARITY)
				SCON = 0xda;                //9位可变波特率,校验位初始为1
		#elif (UART1_PARITYBIT == SPACE_PARITY)
				SCON = 0xd2;                //9位可变波特率,校验位初始为0
		#endif
		
		AUXR |= 0x40;                //定时器1为1T模式
		TMOD |= 0x00;                //定时器1为模式0(16位自动重载)
		TL1 = (65536 - (FOSC/4/baud));   //设置波特率重装值
		TH1 = (65536 - (FOSC/4/baud))>>8;
		TR1 = 1;                    //定时器1开始启动
    ES = 1;                     //使能串口中断
		EA = 1;
	}

	//串口2初始化
	void Init_uart2(unsigned int baud){
		
		#if (UART2_IO == 0)
			P_SW2 &= ~S2_S0;            //S2_S0=0 (P1.0/RxD2, P1.1/TxD2)
		#elif (UART2_IO==1)
			P_SW2 |= S2_S0;           //S2_S0=1 (P4.6/RxD2_2, P4.7/TxD2_2)
		#endif
		#if (UART2_PARITYBIT == NONE_PARITY)
				S2CON = 0x50;               //8位可变波特率
		#elif (UART2_PARITYBIT == ODD_PARITY) || (UART2_PARITYBIT == EVEN_PARITY) || (UART2_PARITYBIT == MARK_PARITY)
				S2CON = 0xda;               //9位可变波特率,校验位初始为1
		#elif (UART2_PARITYBIT == SPACE_PARITY)
				S2CON = 0xd2;               //9位可变波特率,校验位初始为0
		#endif
		T2L = (65536 - (FOSC/4/baud));   //设置波特率重装值
		T2H = (65536 - (FOSC/4/baud))>>8;
		AUXR |= 0x14;                		 //T2为1T模式, 并启动定时器2
		IE2 |= 0x01;                 		 //使能串口2中断
		EA = 1;													 //开启总中断
	}
	
	//发送串口1数据
	void UART1_SendData(unsigned char dat)
	{
			while (tx1_busy);               //等待前面的数据发送完成
			ACC = dat;                  //获取校验位P (PSW.0)
			if (P)                      //根据P来设置校验位
			{
				#if (UART1_PARITYBIT == ODD_PARITY)
								TB8 = 0;                //设置校验位为0
				#elif (UART1_PARITYBIT == EVEN_PARITY)
								TB8 = 1;                //设置校验位为1
				#endif
			}
			else
			{
				#if (UART1_PARITYBIT == ODD_PARITY)
								TB8 = 1;                //设置校验位为1
				#elif (UART1_PARITYBIT == EVEN_PARITY)
								TB8 = 0;                //设置校验位为0
				#endif
			}
			tx1_busy = 1;
			SBUF = ACC;                 //写数据到UART数据寄存器
	}
	//串口1发送字符串
	void UART1_SendString(char *s){
		while (*s)                  //检测字符串结束标志
		{
			UART1_SendData(*s++);         //发送当前字符
		}
	}
	
	//发送串口2数据
	void UART2_SendData(unsigned char dat){
			while(tx2_busy);               //等待前面的数据发送完成
			ACC = dat;                  //获取校验位P (PSW.0)
			if(P)                       //根据P来设置校验位
			{
	#if (UART2_PARITYBIT == ODD_PARITY)
					S2CON &= ~S2TB8;        //设置校验位为0
	#elif (UART2_PARITYBIT == EVEN_PARITY)
					S2CON |= S2TB8;         //设置校验位为1
	#endif
			}
			else
			{
	#if (UART2_PARITYBIT == ODD_PARITY)
					S2CON |= S2TB8;         //设置校验位为1
	#elif (UART2_PARITYBIT == EVEN_PARITY)
					S2CON &= ~S2TB8;        //设置校验位为0
	#endif
			}
			tx2_busy = 1;
			S2BUF = ACC;                //写数据到UART2数据寄存器
	}

	//串口2发送字符串
	void UART2_SendString(char *s){
		while (*s)                  //检测字符串结束标志
		{
			UART2_SendData(*s++);         //发送当前字符
		}
	}
		
	
	//重定义
	char putchar(char ch){
			PRINTF(ch);
			return ch;
	}
	
	//上报打印
	void report_printf(const char *fmt,...)  
	{  
			va_list ap;  
			char xdata string[20];
			va_start(ap,fmt);  
			vsprintf(string,fmt,ap);//此处也可以使用sprintf函数，用法差不多，稍加修改即可，此处略去  
			REPORT(string);  
			va_end(ap);  
	} 

	void UART1_Routine() interrupt 4 using 2
	{
		if (RI)
		{
				RI = 0;                 //清除RI位
				cmd_last = cmd;
				cmd = SBUF;
		}
		if (TI)
		{
				TI = 0;                 //清除TI位
				tx1_busy = 0;               //清忙标志
		} 
	}
	
	//UART2 中断服务程序
	void UART2_Routine() interrupt 8 using 3
	{
		if (S2CON & S2RI)						//接收中断
		{
			S2CON &= ~S2RI;         //清除S2RI位
			
			
		}
		if (S2CON & S2TI)						//发送中断
		{
				S2CON &= ~S2TI;         //清除S2TI位
				tx2_busy = 0;           //清发送忙标志
		}
	}

	/**
		* @brief  线束检测任务
		* @retval 0: 检测完成  其他：错误
		*/
	unsigned char Check_Task(void){
		line_last = line;
		if(SA1||SA2||SA3||SA4)				//A组检测
		{
			pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
			= bus[0] = bus[1] = bus[2] = bus[3] = 0;
			line = 0;
			if (f_debug){
				printf("\r\n debug检测内容：\r\n");
				printf(" A组检测  line = %d\r\n", line);}
			pos[0] = SA1;pos[1] = SA2;pos[2] = SA3;pos[3] = SA4;
			pos[4] = 15 - (pos[0]|pos[1]<<1|pos[2]<<2|pos[3]<<3);
			if (f_debug)
				printf(" SA1=%d; SA2=%d; SA3=%d; SA4=%d; SA_group=%d\r\n", pos[0],pos[1],pos[2],pos[3],pos[4]);
			if(pos[4] > 4) {return 1;}
			bus[0] = A0; bus[1] = A1; bus[2] = A2;
			bus[3] = bus[0]|bus[1]<<1|bus[2]<<2;
			if (f_debug)
				printf(" A0=%d; A1=%d; A2=%d; A210=%d\r\n", bus[0],bus[1],bus[2],bus[3]);
			
			line += (bus[3]+8*pos[4]);
			return 0;
		}
		if(SB1||SB2||SB3||SB4)		//B组检测
		{
			pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
			= bus[0] = bus[1] = bus[2] = bus[3] = 0;
			line = 32;
			if (f_debug)
				printf(" B组检测  line = %d\r\n", line);	
			pos[0] = SB1;pos[1] = SB2;pos[2] = SB3;pos[3] = SB4;
			pos[4] = 15 - (pos[0]|pos[1]<<1|pos[2]<<2|pos[3]<<3);
			if (f_debug)
				printf(" SB1=%d; SB2=%d; SB3=%d; SB4=%d; SB_group=%d\r\n", pos[0],pos[1],pos[2],pos[3],pos[4]);
			if(pos[4] > 4) {return 2;}
			bus[0] = B0; bus[1] = B1; bus[2] = B2;
			bus[3] = bus[0]|bus[1]<<1|bus[2]<<2;
			if (f_debug)
				printf(" B0=%d; B1=%d; B2=%d; B210=%d\r\n", bus[0],bus[1],bus[2],bus[3]);
			line += (bus[3]+8*pos[4]);
			return 0;
		}
		if(SC1||SC2||SC3||SC4)		//C组检测
		{
			pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
			= bus[0] = bus[1] = bus[2] = bus[3] = 0;
			line = 64;
			if (f_debug)
				printf(" C组检测  line = %d\r\n", line);
			pos[0] = SC1;pos[1] = SC2;pos[2] = SC3;pos[3] = SC4;
			pos[4] = 15 - (pos[0]|pos[1]<<1|pos[2]<<2|pos[3]<<3);
			if (f_debug)
				printf(" SC1=%d; SC2=%d; SC3=%d; SC4=%d; SC_group=%d\r\n", pos[0],pos[1],pos[2],pos[3],pos[4]);
			if(pos[4] > 4) {return 3;}
			bus[0] = C0; bus[1] = C1; bus[2] = C2;
			bus[3] = bus[0]|bus[1]<<1|bus[2]<<2;
			if (f_debug)
				printf(" C0=%d; C1=%d; C2=%d; C210=%d\r\n", bus[0],bus[1],bus[2],bus[3]);
			line += (bus[3]+8*pos[4]);
			return 0;
		}
		if(SD1||SD2||SD3||SD4)		//D组检测
		{
			pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
			= bus[0] = bus[1] = bus[2] = bus[3] = 0;
			line = 96;
			if (f_debug)
				printf(" D组判断  line = %d\r\n", line);
			pos[0] = SD1;pos[1] = SD2;pos[2] = SD3;pos[3] = SD4;
			pos[4] = 15 - (pos[0]|pos[1]<<1|pos[2]<<2|pos[3]<<3);
			if (f_debug)
				printf(" SD1=%d; SD2=%d; SD3=%d; SD4=%d; SD_group=%d\r\n", pos[0],pos[1],pos[2],pos[3],pos[4]);
			if(pos[4] > 4) {return 4;}
			bus[0] = D0; bus[1] = D1; bus[2] = D2;
			bus[3] = bus[0]|bus[1]<<1|bus[2]<<2;
			if (f_debug)
			printf(" D0=%d; D1=%d; D2=%d; D210=%d\r\n", bus[0],bus[1],bus[2],bus[3]);
			line += (bus[3]+8*pos[4]);
			return 0;
		}
		if(SE1||SE2||SE3)		//E组检测
		{
			pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
			= bus[0] = bus[1] = bus[2] = bus[3] = 0;
			line = 128;
			if (f_debug)
			printf(" E组检测  line = %d\r\n", line);
			pos[0] = SE1;pos[1] = SE2;pos[2] = SE3;
			pos[4] = 15 - (pos[0]|pos[1]<<1|pos[2]<<2);
			if (f_debug)
			printf(" SE1=%d; SE2=%d; SE3=%d; SE_group=%d\r\n", pos[0],pos[1],pos[2],pos[4]);
			if(pos[4] > 3) {return 5;}
			bus[0] = E0; bus[1] = E1; bus[2] = E2;
			bus[3] = bus[0]|bus[1]<<1|bus[2]<<2;
			if (f_debug)
			printf(" E0=%d; E1=%d; E2=%d; E210=%d\r\n", bus[0],bus[1],bus[2],bus[3]);
			line += (bus[3]+8*pos[4]);
			return 0;
		}
		return 0;
	}

	/**
		* @brief  初始化打印
		*/
	void Printf_Init(void){
		char i;
		printf("\r\n");
		printf("Loading...\r\n");
		for(i=0;i<50;i++)
		{
			printf("=");
			delay_us(50);
		}
		printf("\r\n");
		printf(" _    _             ___ _           _    \r\n");
		printf("| |  (_)_ _  ___   / __| |_  ___ __| |__ \r\n"); 
		printf("| |__| | ' \\/ -_) | (__| ' \\/ -_) _| / / \r\n");
		printf("|____|_|_||_\\___|  \\___|_||_\\___\\__|_\\_\\ \r\n");
		printf("\r\n");
		printf("Version:			V1.0.1	    \r\n");
		printf("Author:				Gsx   	    \r\n");
		printf("芯片信息:     STC15F2K60S2\r\n");
		printf("系统主频:     %ld\r\n", FOSC);
		printf("波特率:    	  %d\r\n", UART2_BODE);
		printf("输入0x04查看指令帮助      \r\n");
	}
	/**
		* @brief  报错打印
		*/
	void Err_printf(void){
		switch(err)
		{
			case 1:
			{
				printf("\r\nError(1): SA1 ~ SA4线出错,已停止检测\r\n");
			}break;
			case 2:
			{
				printf("\r\nError(2): SB1 ~ SB4线出错,已停止检测\r\n");
			}break;
			case 3:
			{
				printf("\r\nError(3): SC1 ~ SC4线出错,已停止检测\r\n");
			}break;
			case 4:
			{
				printf("\r\nError(4): SD1 ~ SD4线出错,已停止检测\r\n");
			}break;
			case 5:
			{
				printf("\r\nError(5): SE1 ~ SE3线出错,已停止检测\r\n");
			}break;
			default:
			break;
		}
	}

	/**
		* @brief  初始化任务
		*/
	void Init_Task(void){
		
		Init_uart2(UART2_BODE);
		Init_uart1(UART1_BODE);
		Printf_Init();
	}

	/**
		* @brief  打印任务
		*/
	void Printf_Task(void){
			if(!err){
				if(line==-1)
				{
					printf("\r\n没有线被拉低。\r\n");
					report_printf("ff");
				}
				else{
					printf("\r\n%d线被拉低。\r\n", line);
					report_printf("%d",(int)line);		//上报
					line = -1;
				}
			}else
			{
				Err_printf();
				report_printf("ff%d", (int)err);
			}
	}
	
	void main(void)
	{
		Init_Task();
		while (1)
		{	
			if(cmd!=BOOT)
			{
				switch (cmd)
				{
					case STEP:
					{
						cmd = BOOT;
						err_last = err;
						err = Check_Task();
						Printf_Task();
					}
					break;
					case ROLL:
					{
						err_last = err;
						err = Check_Task();
						Printf_Task();
					}
					break;
					case DEBUG:
					{
						f_debug = ~f_debug;
						if(f_debug)printf("\r\nDEBUG已开启\r\n");
						else printf("\r\nDEBUG已关闭\r\n");
						cmd = cmd_last;
					}
					break;
					case HELP:
					{
						printf("\r\n");
						printf("*============ CMD LIST ============*\r\n");
						printf(" 0x00----------BOOT停止指令         \r\n");
						printf(" 0x01----------STEEP单步测试指令    \r\n");
						printf(" 0x02----------ROLL滚动测试指令     \r\n");
						printf(" 0x03----------DEBUG开关debug指令   \r\n");
						printf(" 0x04----------HELP指令列表         \r\n");
						printf(" 0x05----------REBOOT重启         	\r\n");
						printf(" 0x06----------AUTO自动检测        	\r\n");
						cmd = BOOT;
					}
					break;
					case REBOOT:
					{
						pos[0] = pos[1] = pos[2] = pos[3] = pos[4]
						= bus[0] = bus[1] = bus[2] = bus[3] = 0;
						line = 0;
						Printf_Init();
						cmd = BOOT;
					}
					break;
					case AUTO:
					{
						err_last = err;
						err = Check_Task();
						
						if(err_last!=err || line_last!= line)
						{
							Printf_Task();
						}
					}
					break;
					default:
						break;
				}
			}
			
		}
	}
