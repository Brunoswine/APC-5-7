#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>

unsigned int sleepTime = 0;

char date[6];

unsigned int registers[] = { 0x00, 0x02, 0x04, 0x07, 0x08, 0x09 };
void interrupt(*oldRTChandler)(...);
void interrupt(*oldAlarm) (...);

void getTime();
void setTime();
void delay();
void setAlarm();
void resetAlarm();
void inputTime();
int decimalToBcd(int decimal);


void playBeep()  // бип
{
	int k;
	outp(0x43, 0xB6);
	k = 1193180 / 1000;
	outp(0x42, k % 256);
	outp(0x42, k / 256);

	k = inp(0x61);
	outp(0x61, k | 0x03);
	delay(500);
	k = inp(0x61);
	outp(0x61, k & 0xFC);
}

void interrupt newAlarm(...)
{
	puts("Alarm");
	oldAlarm();
	resetAlarm();
}

int main()
{
	int delayMillisecond;
	oldAlarm = getvect(0x4A); //переопределение прерывания от будильника
	while (1)
	{
		printf("1 - Current time\n");
		printf("2 - Set time\n");
		printf("3 - Set alarm\n");
		printf("4 - Set delay\n");
		printf("q - EXIT\n");


		switch (getch())
		{
		case '1':
		{
			system("cls");
			getTime();
			break;
		}
		case '2':
		{
			setTime();
			break;
		}
		case '3':
		{
			setAlarm();
			break;
		}
		case '4':
		{
			delay();
			break;
		}
		case 'q':
		{
			exit(1);
		}
		default:
		{
			system("cls");
			break;
		}
		}
	}
}

void getTime()
{

	int i = 0;
	for (i = 0; i < 6; i++)
	{
		outp(0x70, registers[i]); // выбор регистра
		date[i] = inp(0x71); // считать значение
	}

	printf("Current time(hh:mm:ss) - %x:%x:%x\n", date[2], date[1], date[0]);
	printf("Current date(DD:MM::YY) - %x.%x.%2x\n", date[3], date[4], date[5]);
}

void setTime()
{
	inputTime();
	_disable(); // запретить прерывния
	outp(0x70, 0xA);
	int attemptNum = 10;
	int i = 0;
	for (; i < attemptNum; ++i)
	{
		if(!(inp(0x71) & 0x80)) break;
	}

	if (i == attemptNum) 
	{
		printf("Install RTC time error\n");
		return;
	}

	outp(0x70, 0xB); //работа с часами
	outp(0x71, inp(0x71) | 0x80);	// отключить внутренний цикл обновления часов

	for (int j = 0; j < 3; j++) 
	{
		outp(0x70, registers[j]);	// выбрать регистр
		outp(0x71, date[j]); 	// записать значения
	}

	outp(0x70, 0xB); //работа с часами
	outp(0x71, inp(0x71) & 0x7F);	// разрешить цикл обновления часов
	_enable(); //включение работы прерываний
	system("cls");
}

void interrupt newRTChandler(...)
{
	sleepTime--;

	oldRTChandler();
}

void delay()
{
	printf("Input delay time (milliseconds): ");
	do {
		fflush(stdin);
	} while(!scanf("%u", &sleepTime));
        if (sleepTime<=0){
	    printf("Wrong input\n");
            return;
        }
	_disable(); // запретить прерывния
	oldRTChandler = getvect(0x70);
	setvect(0x70, newRTChandler); //перезапись прерывания на новое значение
	_enable();

	getTime();
        printf("Waiting....");
	outp(0xA1, inp(0xA1) & 0xFE);													

	outp(0x70, 0xB);																	
	outp(0x71, inp(0x71) | 0x40);

	while (sleepTime);

	outp(0xA1, (inp(0xA1) | 0x01));	

	_disable();
	setvect(0x70, oldRTChandler);
	_enable();
	getTime();


	return;
}

void setAlarm()
{
	inputTime();

	_disable(); // запретить прерывния

	outp(0x70, 0xA);
	int attemptNum = 10;
	int i = 0;
	for (; i < attemptNum; ++i)
	{
		if(!(inp(0x71) & 0x80)) break;
	}

	if (i == attemptNum) 
	{
		printf("Error\n");
		return;
	}
	unsigned int regs = 0x05;
	for (i = 2; i >= 0; i--, regs-=2)
	{
		outp(0x70, regs);
		outp(0x71, date[i]);	// ввод времени
	}
	outp(0x70, 0xB);
	outp(0x71, (inp(0x71) | 0x20));
	setvect(0x4A, newAlarm); // новое прерывание

	outp(0xA1, (inp(0xA0) & 0xFE)); 
	_enable();
}

void resetAlarm()
{
	if (oldAlarm == NULL) return;

	_disable();

	outp(0x70, 0xA);
	int attemptNum = 10;
	int i = 0;
	for (; i < attemptNum; ++i)
	{
		if(!(inp(0x71) & 0x80)) break;
	}

	if (i == attemptNum) 
	{
		printf("Error\n");
		return;
	}

	outp(0x70, 0xB);																	
	outp(0x71, (inp(0x71) & 0xDF));														

	setvect(0x4A, oldAlarm);															

	if(sleepTime > 0) 
	{
		sleepTime++;
		_enable();
		return;
	}														
	outp(0xA1, (inp(0xA1) | 0x01));														

	_enable();
}

void inputTime()
{
	int n;

	do {
		fflush(stdin);
		printf("Input hours: ");
	} while (!scanf("%d", &n) || !(n < 23 && n >= 0));
	date[2] = decimalToBcd(n);

	do {
		fflush(stdin);
		printf("Input minutes: ");
	} while (!scanf("%d", &n) || !(n < 59 && n >= 0));
	date[1] = decimalToBcd(n);

	do {
		fflush(stdin);
		printf("Input seconds: ");
	} while (!scanf("%d", &n) || !(n < 59 && n >= 0));
	date[0] = decimalToBcd(n);
}

int decimalToBcd(int decimal)
{
	return ((decimal / 10 * 16) + (decimal % 10));
}
