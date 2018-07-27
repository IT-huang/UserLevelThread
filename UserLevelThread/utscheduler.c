#include <stdlib.h>
#include "utscheduler.h"
#include "utexcept.h"

/*
��ÿ�ε�����ĳ���߳�һ�κ󣬶��������һ�µ�ǰ�̵߳�״̬���Ա㼰ʱ��˯�ߡ�ִ������߳��л�����Ӧ���� <?ò���ö��и���һЩ��?> ��
*/
void checkCurrent(uThd* prev,uThd* current,utSch sch) {
	if ((*current)->status == UtStatusNormal) {
		//�����������������sleep��finish���¶��ᵼ��Ѱ����һ����ִ���̱߳�����prev->next������������ô������ͳһ
		//�л�Ϊ���л����
		(*prev) = (*prev)->next;
		return;
	}
	if ((*current)->status == UtStatusSleep) {
		(*prev)->next = (*current)->next;
		(*current)->next = sch->uThdSleep->next;
		sch->uThdSleep->next = (*current);
		return;
	}
	if ((*current)->status == UtStatusFinish){
		(*prev)->next = (*current)->next;
		(*current)->next = sch->uThdFinish->next;
		sch->uThdFinish->next = (*current);
		return;
	}
}

/*
���˯���߳�����û�и�������
*/
void checkSleepUthd(utSch sch) {
	uThd utSleepPrev = sch->uThdSleep;
	uThd utSleep = sch->uThdSleep->next;
	while (utSleep != NULL) {
		DWORD curMils = GetTickCount();
		if (curMils > (utSleep->sleepTime + utSleep->sleepDuri)) {
			utSleepPrev->next = utSleep->next;
			utSleep->next = sch->uThdNormal->next;
			sch->uThdNormal->next = utSleep;
			utSleep = utSleepPrev->next;
		}
		else {
			utSleep = utSleep->next;
			utSleepPrev = utSleepPrev->next;
		}
	}
}

utSch utSchedulerCreate() {
	utSch sch;
	sch = (utSch)malloc(sizeof(struct uThreadScheduler));
	sch->uThdNormal = (uThd)malloc(sizeof(struct uThread));
	sch->uThdNormal->next = NULL;
	sch->uThdSleep = (uThd)malloc(sizeof(struct uThread));
	sch->uThdSleep->next = NULL;
	sch->uThdFinish = (uThd)malloc(sizeof(struct uThread));
	sch->uThdFinish->next = NULL;
	return sch;
}

void utSchedulerDestory(utSch sch) {
	uThd temp1 = sch->uThdNormal->next, temp2;
	while (temp1){
		temp2 = temp1->next;
		uThreadDestory(temp1);
		temp1 = temp2;
	}
	temp1 = sch->uThdSleep->next;
	while (temp1) {
		temp2 = temp1->next;
		uThreadDestory(temp1);
		temp1 = temp2;
	}
	temp1 = sch->uThdFinish->next;
	while (temp1) {
		temp2 = temp1->next;
		uThreadDestory(temp1);
		temp1 = temp2;
	}
	free(sch->uThdNormal);
	free(sch->uThdSleep);
	free(sch->uThdFinish);
	free(sch);
}

void utSchAddUthd(utSch sch, uThd thd) {
	thd->next = sch->uThdNormal->next;
	sch->uThdNormal->next = thd;
}

void utSchStart(utSch sch) {
	uThd prev = sch->uThdNormal;//ָ��current��ǰһ���������ƶ��߳�
	uThd current = sch->uThdNormal->next;//ȡ�����������еĵ�һ���߳�
	if (current==NULL){//���Ϊ�գ������û���߳�
		printf("-------------------------���û����߳�-------------------------\n");
		return;
	}

	while (sch->uThdNormal->next!=NULL||sch->uThdSleep->next!=NULL){//�����������˯�������Ϊ��ʱ������ѭ��

		if (current != NULL) {//���ܴ���normal��Ϊ�գ�sleep��Ϊ��
			//�൱��try
			_asm {
				//�����쳣����ǰ�Ļ���
				mov dword ptr[exregs + 0h], ebp;
				mov dword ptr[exregs + 8h], ebx;
				mov dword ptr[exregs + 0ch], edi;
				mov dword ptr[exregs + 10h], esi;
				mov dword ptr[exregs + 14h], offset cachec;//eip
				//�쳣������
				push _except_handler;
				push fs : [0];
				mov fs : [0], esp;
				mov dword ptr[exregs + 4h], esp;//esp
			}
			//��ʼ��ָ��߳�
			utResume(current);
			//�൱��cache
			_asm {
				jmp rightc;//��������£�ֱ�������ÿ�
			cachec:
				mov eax, dword ptr[current];
				mov dword ptr[eax + 34h], 2;//����ǰ�߳̽���
			rightc:
				mov eax, [esp];
				mov fs : [0], eax;
				add esp, 8;
			}
			//��⵱ǰ�߳�״̬
			checkCurrent(&prev, &current, sch);
		}

		//���˯���߳�
		checkSleepUthd(sch);

		//�Ӿ���������ȡ��һ���߳�
		if (prev == NULL)
			prev = sch->uThdNormal;
		current = prev->next;//ִ��checkcurrent֮��prev�Ѿ�����˴˴���ȷ��prev
		if (current == NULL) {
			prev = sch->uThdNormal;
			current = sch->uThdNormal->next;
		}

	}
}