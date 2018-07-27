#include <stdlib.h>
#include "uthread.h"


/*
�������


���ú���ʱ  call func
push eip
jump func

���ú���  �в��� func��int i�� 
push i;
call func;


func(){
	esp



����ִ����ϵ�ʱ��  ret
pop eip


}

*/

uThd current;//���浱ǰ�˳̵�ָ�룬�Ա�yield�ж�����б������

uThd uThreadCreated(void *stack, int stackSize, function func) {
	uThd u = (struct uThread *) malloc(sizeof(struct uThread));
	u->stack = stack;
	u->stackSize = stackSize;
	u->regs[1] = (int)stack + stackSize - 4;//espҪָ���ջ��ĩ�ˣ���Ϊ��ջÿ��push���ڼ�С
	u->hasRun = 0;
	u->status = UtStatusNormal;//����
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

//��ʼ��ָ������˳�
void NAKED utResume(uThd u) {
	/*
	
	u->hasrun 
	û�����й� call func ��һ�ε���
	
	�Ѿ����й�  

		�Ƿ����  ֱ���˳�
		δִ����  ��u-��regs�ָ������� ��������
	
	
	*/



	_asm {
		/*ʱ��ȷ��ecx�ǲ���u*/
		mov ecx, dword ptr[esp + 4];//��ȡ����u

		//�������resumeʱ���ⲿ��Ϣ
		mov dword ptr[pregs + 0h], ebp;
		mov dword ptr[pregs + 8h], ebx;
		mov dword ptr[pregs + 0ch], edi;
		mov dword ptr[pregs + 10h], esi;
		pop eax;//��ȡeip
		mov dword ptr[pregs + 14h], eax;//����eip
		mov dword ptr[pregs + 4h], esp;//����esp

		//current=u
		mov dword ptr[current], ecx;

		//�ж��Ƿ����й�
		mov eax, dword ptr[ecx + 30h];//��ȡu->hasRun
		cmp eax, 1;
		jz runned;//���й�
				 //δ���й� ������ջ����call func
		mov dword ptr[ecx + 30h], 1;//��u->hasRun=1;

		mov eax, dword ptr[ecx + 4h];//��ȡ��ջ�ĵ׶ˣ����ֵ������uthd->regs[1]---->esp��
		mov esp, eax;//������ջ
		//mov esi, dword ptr[ecx + 10h];//�������Ҫ�޸�esi������ᱨ�� ---ummm����Ҳû��Ҫ�л�
									  //-1073741788,   "{��������}/n����Ĳ���������Ķ�����������������ָ���Ķ������Ͳ����ϡ�"
		mov eax, dword ptr[ecx + 18h];//��ȡfunc
		push ecx;//Ҫʱ�̱�֤ecx=����u����ecxѹջ��ȷ���䲻�ᶪʧ
		call eax;
		pop ecx;//��ջ���ָ�ecx
		mov dword ptr[ecx + 34h], 2;//��ִ�е�����ض��Ǻ�����ִ����ϣ�ret�����ˣ�������u->status=finish;

	runned://������й������������
		mov eax, dword ptr[ecx + 34h];//��ȡstatus
		cmp eax, 2;
		jz myexit;//�����й������Ѿ�ִ�����

		//�����й�����δִ�����
		//�ָ��Ĵ�������������

		//TODO �˴����Է�װ��һ������
		mov ebp, dword ptr[ecx];
		mov esp, dword ptr[ecx + 4h];
		mov ebx, dword ptr[ecx + 08h];
		mov edi, dword ptr[ecx + 0ch];
		mov esi, dword ptr[ecx + 10h];
		mov eax, dword ptr[ecx + 14h];//eip  ����ֱ�Ӷ�eip��ֵ  ��jmp
		jmp eax;

	myexit://�л�Ϊ�������ļĴ�������������		��Ų�������������֣�����˵exit
		mov ebp, dword ptr[pregs + 0h];
		mov esp, dword ptr[pregs + 4h];
		mov ebx, dword ptr[pregs + 8h];
		mov edi, dword ptr[pregs + 0ch];
		mov esi, dword ptr[pregs + 10h];
		mov eax, dword ptr[pregs + 14h];
		jmp eax;

	}

	/**
	mov ecx, dword ptr[u]
	pop eax
	mov dword ptr[ecx + 24h], eax;//edi->pRegs[3]
	pop eax
	mov dword ptr[ecx + 28h], eax;//esi->pRegs[4]
	pop eax
	mov dword ptr[ecx + 20h], eax;//ebx->pRegs[2]
	mov esp, ebp
	pop eax
	mov dword ptr[ecx + 18h], eax;//ebp->pRegs[0]
	pop eax
	mov dword ptr[ecx + 2ch], eax;//save eip

	; pop ecx;//��������ѹջ
	mov dword ptr[ecx + 1ch], esp;//esp->pRegs[1]
	}
	current = u;
	//׼�������˳�
	//δ���й�
	if (!u->hasRun) {
	u->hasRun = 1;
	_asm {
	mov ecx, dword ptr[u]
	mov eax, dword ptr[ecx + 4h];//get stack end
	mov esp, eax;//change stack
	mov esi, dword ptr[ecx + 10h]//�������Ҫ�޸�esi������ᱨ��
	//-1073741788,   "{��������}/n����Ĳ���������Ķ�����������������ָ���Ķ������Ͳ����ϡ�"
	mov eax, dword ptr[ecx + 30h];//get func
	call eax
	//pop ebp
	}
	//TODO:�ָ�esp��stack
	u->exit = 1;
	}
	//�����й�
	//�����н��� �������ⲿȥ�������������˳�
	if (u->exit) {
	//TODO: swap();
	_asm {
	mov ecx, dword ptr[u]
	mov ebp, dword ptr[ecx + 18h]
	mov esp, dword ptr[ecx + 1ch]
	mov ebx, dword ptr[ecx + 20h]
	mov edi, dword ptr[ecx + 24h]
	mov esi, dword ptr[ecx + 28h]
	mov eax, dword ptr[ecx + 2ch]
	jmp eax
	}
	}

	//�ѿ�ʼִ�У�����δ����
	//�ָ��Ĵ���������ִ��
	_asm {
	mov ecx, dword ptr[u]
	mov ebp, dword ptr[ecx]
	mov esp, dword ptr[ecx + 4h]
	mov ebx, dword ptr[ecx + 08h]
	mov edi, dword ptr[ecx + 0ch]
	mov esi, dword ptr[ecx + 10h]
	mov eax, dword ptr[ecx + 14h]
	jmp eax
	}
	/**/
}

void NAKED utYield() {
	_asm {
		//�����߳�������
		mov ecx, dword ptr[current];//��ȡ��ǰ���е��߳�
		mov dword ptr[ecx], ebp;
		mov dword ptr[ecx + 8h], ebx;
		mov dword ptr[ecx + 0ch], edi;
		mov dword ptr[ecx + 10h], esi;
		pop eax;//eip
		mov dword ptr[ecx + 14h], eax;//eip
		mov dword ptr[ecx + 4h], esp;//esp

		//�ָ������������������������˳�
		mov ebp, dword ptr[pregs + 0h];
		mov esp, dword ptr[pregs + 4h];
		mov ebx, dword ptr[pregs + 8h];
		mov edi, dword ptr[pregs + 0ch];
		mov esi, dword ptr[pregs + 10h];
		mov eax, dword ptr[pregs + 14h];//eip
		jmp eax;
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
		mov ecx, dword ptr[current];
		mov dword ptr[ecx], ebp;
		mov dword ptr[ecx + 8h], ebx;
		mov dword ptr[ecx + 0ch], edi;
		mov dword ptr[ecx + 10h], esi;
		pop eax;
		mov dword ptr[ecx + 14h], eax;//eip
		mov dword ptr[ecx + 4h], esp;

		//����˯���������
		//current->status = UtStatusSleep;
		//current->sleepDuri = duri;
		//DWORD curMils = GetTickCount();
		//current->sleepTime = curMils;
		mov dword ptr[ecx + 34h], 1;//status=sleep
		mov eax, dword ptr[esp];//����duri
		mov dword ptr[ecx + 28h], eax;//sleepduri=duri;
		call GetTickCount;//��ȡ��ǰʱ�䣬����ֵ��eax��
		mov dword ptr[ecx + 24h], eax;//sleeptime=time;

		//����yield���ָ������������������������˳�
		mov ebp, dword ptr[pregs + 0h];
		mov esp, dword ptr[pregs + 4h];
		mov ebx, dword ptr[pregs + 8h];
		mov edi, dword ptr[pregs + 0ch];
		mov esi, dword ptr[pregs + 10h];
		mov eax, dword ptr[pregs + 14h];
		jmp eax;
	}
}