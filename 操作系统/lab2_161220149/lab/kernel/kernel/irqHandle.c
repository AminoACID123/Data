#include "x86.h"
#include "device.h"

uint32_t vp= 0;

void syscallHandle(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) {
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */

	switch(tf->irq) {
	
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}
void printc(unsigned char ch){
	if(ch=='\n'){
		vp = (vp/160+1)*160;
		return;
	}
	unsigned short tmp = ch+0x0c00;
	asm volatile ("movl %%ebx, %%edi"::"b"(vp));
	asm volatile ("movw %0, %%ax"::"a"(tmp));
	asm volatile ("movw %ax, %gs:(%edi)");
	vp+=2;
}
int puts(struct TrapFrame *tf){
	asm volatile ("movw %%ax, %%gs"::"a"KSEL(SEG_VIDEO));
	int len=0;
	for(int i=0;i<tf->edx;i++){
		if(*(char *)(tf->ecx+i)!='\0'){
			printc(*(char *)(tf->ecx+i));
			len++;
		}
		else break;

	}
	return len;
}
void syscallHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	if(tf->eax==0)
		tf->eax = puts(tf);
}



void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
