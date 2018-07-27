#include <stdlib.h>
#include "uthread.h"

uThd current;//���浱ǰ�˳̵�ָ�룬�Ա�yield�ж�����б������

uThd uThreadCreated(void *stack, int stackSize, function func) {
	uThd u = (struct uThread *) malloc(sizeof(struct uThread));
	u->stack = stack;
	u->stackSize = stackSize;
	u->regs[1] = (int)stack + stackSize - 4;//espҪָ���ջ��ĩ�ˣ���Ϊ��ջÿ��push���ڼ�С
	u->status = UtStatusUnstart;//����
	u->func = func;//����� �������
	u->next = NULL;
	return u;
}

uThd uThreadCreate(function func) {
	int stackSize = 4 * 1024 * 1024;//Ĭ�Ϸ���4MB�Ķ�ջ�ռ�
	char *stack = NULL;
	stack = (char*)malloc(sizeof(char)*stackSize);
	return uThreadCreated(stack, stackSize, func);
}

void uThreadDestory(uThd u) {
	free(u->stack);
	free(u);
}

//����������ļĴ�������
void NAKED pushPregs() {
	_asm {
		pop edx;

		//�������resumeʱ���ⲿ��Ϣ
		mov dword ptr[pregs + 0h], ebp;
		mov dword ptr[pregs + 8h], ebx;
		mov dword ptr[pregs + 0ch], edi;
		mov dword ptr[pregs + 10h], esi;
		pop eax;//��ȡeip
		mov dword ptr[pregs + 14h], eax;//����eip
		mov dword ptr[pregs + 4h], esp;//����esp

		jmp edx;
	}
}
//��ecx�лָ��߳����ݣ������߳̿�ʼ����
void NAKED popEcx() {
	_asm {
		pop edx;

		mov ebp, dword ptr[ecx];
		mov esp, dword ptr[ecx + 4h];
		mov ebx, dword ptr[ecx + 08h];
		mov edi, dword ptr[ecx + 0ch];
		mov esi, dword ptr[ecx + 10h];
		mov eax, dword ptr[ecx + 14h];//eip  ����ֱ�Ӷ�eip��ֵ  ��jmp
		jmp eax;
	}
}
//��pregs�лָ��������Ĵ��������õ�������ʼ����
void NAKED popPregs() {
	_asm {
		pop edx;

		mov ebp, dword ptr[pregs + 0h];
		mov esp, dword ptr[pregs + 4h];
		mov ebx, dword ptr[pregs + 8h];
		mov edi, dword ptr[pregs + 0ch];
		mov esi, dword ptr[pregs + 10h];
		mov eax, dword ptr[pregs + 14h];
		jmp eax;
	}
}
//���Ĵ������浽ecx��
void NAKED pushEcx() {
	_asm {
		pop edx;

		mov ecx, dword ptr[current];//��ȡ��ǰ���е��߳�
		mov dword ptr[ecx], ebp;
		mov dword ptr[ecx + 8h], ebx;
		mov dword ptr[ecx + 0ch], edi;
		mov dword ptr[ecx + 10h], esi;
		pop eax;//eip
		mov dword ptr[ecx + 14h], eax;//eip
		mov dword ptr[ecx + 4h], esp;//esp

		jmp edx;
	}
}

//��ʼ��ָ������˳�
void NAKED utResume(uThd u) {
	_asm {
		/*ʱ��ȷ��ecx�ǲ���u*/
		mov ecx, dword ptr[esp + 4];//��ȡ����u

		call pushPregs;

		//current=u
		mov dword ptr[current], ecx;

		//�ж��Ƿ����й�
		mov eax, dword ptr[ecx + 30h];//��ȡu->hasRun
		cmp eax, UtStatusUnstart;
		ja runned;//���й�
				 //δ���й� ������ջ����call func
		mov dword ptr[ecx + 30h], UtStatusNormal;//��u->hasRun=1;status=normal

		mov eax, dword ptr[ecx + 4h];//��ȡ��ջ�ĵ׶ˣ����ֵ������uthd->regs[1]---->esp��
		mov esp, eax;//������ջ
		mov eax, dword ptr[ecx + 18h];//��ȡfunc
		push ecx;//Ҫʱ�̱�֤ecx=����u����ecxѹջ��ȷ���䲻�ᶪʧ
		call eax;
		pop ecx;//��ջ���ָ�ecx
		mov dword ptr[ecx + 30h], UtStatusFinish;//��ִ�е�����ض��Ǻ�����ִ����ϣ�ret�����ˣ�������u->status=finish;

	runned://������й������������
		mov eax, dword ptr[ecx + 30h];//��ȡstatus
		cmp eax, UtStatusFinish;
		jz myexit;//�����й������Ѿ�ִ�����

		//�����й�����δִ�����
		//�ָ��Ĵ�������������
		call popEcx;

	myexit://�л�Ϊ�������ļĴ�������������		��Ų�������������֣�����˵exit
		call popPregs;

	}
}

void NAKED utYield() {
	_asm {
		////�����߳�������
		call pushEcx;

		//�ָ������������������������˳�
		call popPregs;
	}
}

/*
˯�ߺ���
��ʵ�����൱��yield��ֻ�Ƕౣ����˯�߿�ʼʱ��ͳ���ʱ��
����ڵ������л�����е�˯���߳̽��м�⣬�ж����Ƿ�Ҫ�ָ�����
*/
void NAKED utSleep(DWORD duri) {
	_asm {
		//����yield������������
		call pushEcx;

		//����˯���������
		//current->status = UtStatusSleep;
		//current->sleepDuri = duri;
		//DWORD curMils = GetTickCount();
		//current->sleepTime = curMils;
		mov dword ptr[ecx + 30h], UtStatusSleep;//status=sleep
		mov eax, dword ptr[esp];//����duri
		mov dword ptr[ecx + 28h], eax;//sleepduri=duri;
		call GetTickCount;//��ȡ��ǰʱ�䣬����ֵ��eax��
		mov dword ptr[ecx + 24h], eax;//sleeptime=time;

		//����yield���ָ������������������������˳�
		call popPregs;
	}
}