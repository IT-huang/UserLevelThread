#include <stdio.h>
#include <Windows.h>
#include "uthread.h"
#include "utscheduler.h"

#define CYCLES 100//���Ʋ��Ժ���ѭ������
#define PRINT 1//�����Ƿ��ӡ���
int recursion = CYCLES / 2;//�ݹ����

/*
��ͨ���Ժ���1
*/
void normalFunc1() {
	int i = 0;
	while (i<CYCLES){
		if (PRINT)
			printf("Normal Func1: Cycle%d.\n", i);
		utYield();
		i++;
	}
}

/*
��ͨ���Ժ���2
�ݲ�֧�ִ���
*/
void normalFunc2() {
	int i = 0;
	while (i<CYCLES){
		float f = 1.8*3.6;
		if (PRINT)
			printf("Normal Func2: Cycle%d,%f.\n", i,f);
		utYield();
		i++;
	}
}

/*
�ݹ���Ժ���
*/
void recurFunc() {
	int i = recursion;

	if (PRINT)
		printf("Recursion Func: Recursion%d.\n", i);
	recursion--;
	utYield();

	if (recursion == 0)return;
	recurFunc();

	utYield();
	if (PRINT)
		printf("Normal Func2: Recursion%d.\n", i);
}

/*
˯�߲��Ժ���
*/
void sleepFunc() {
	//��ȡ��ǰʱ�䣬����ӡ
	DWORD mils = GetTickCount();
	SYSTEMTIME time;
	GetSystemTime(&time);
	if(PRINT)
		printf("Sleep Func: Current Time: %d: Min-%d, Sec-%d\n", mils, time.wMinute, time.wSecond);

	//˯��
	utSleep(5000);
	
	mils = GetTickCount();
	GetSystemTime(&time);
	if (PRINT)
		printf("Sleep Func: Current Time: %d: Min-%d, Sec-%d\n", mils, time.wMinute, time.wSecond);
}

/*
�쳣���Ժ���
*/
void exceptFunc() {
	if (PRINT)
		printf("Exception Func: Make Error.\n");
	_asm{
		xor eax, eax;
		mov [eax], 0;//���ַΪ0����ֵ�������쳣
	}
	if (PRINT)
		printf("Exception Func: Error Clear.\n");
}

/*
�쳣���Ժ���
*/
void exceptFunc2() {
	if (PRINT)
		printf("Exception Func: Make Error Divid 0.\n");
	int zero = 0;
	int a = 5 / zero;
	if (PRINT)
		printf("Exception Func: Error Clear.\n");
}

utSch sch;//�̵߳�����

void init() {
	sch = utSchedulerCreate();
	utSchAddUthd(sch, uThreadCreate(normalFunc1));
	utSchAddUthd(sch, uThreadCreate(normalFunc2));
	utSchAddUthd(sch, uThreadCreate(recurFunc));
	utSchAddUthd(sch, uThreadCreate(sleepFunc));
	utSchAddUthd(sch, uThreadCreate(exceptFunc));
	utSchAddUthd(sch, uThreadCreate(exceptFunc2));
}

int add(int op1, int op2) {
	int res = op1 + op2;
	return res;
}

int main() {
	//int res = add(1, 2);

	/**/
	init();
	printf("-------------------------Test Start-------------------------\n");
	utSchStart(sch);//��ʼ����
	utSchedulerDestory(sch);//���ٵ��������ͷ��ڴ�
	printf("-------------------------Test End-------------------------\n");
	/**/

	system("pause");
	return 0;
}