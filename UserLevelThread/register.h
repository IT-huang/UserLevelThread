#pragma once

#define UtStatusUnstart  0
#define UtStatusNormal  1
#define UtStatusSleep  2
#define UtStatusFinish  3

#define NAKED _declspec(naked)//�㺯��

typedef int registers[6];//ebp 0h,esp 4h,ebx 8h,edi ch,esi 10h,eip 14h