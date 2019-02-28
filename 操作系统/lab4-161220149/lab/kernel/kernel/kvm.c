#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];


#define SECTSIZE 512

extern void printc(char);
extern void idle();
extern uint32_t vp;

void waitDisk(void) {
	while((inByte(0x1F7) & 0xC0) != 0x40); 
}

void readSect(void *dst, int offset) {
	int i;
	waitDisk();
	
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}

void initSemaphore(){
	for(int i=0;i<10;i++)
		Sem[i].used=0;
	

}

void initPCB(){

	for(int i=2;i<MAX_PCB_NUM;i++)
		PCB[i].state=STATE_DEAD;
	
	PCB[1].pid=1;
	PCB[1].sleepTime=-1;
	PCB[1].timeCount=1;
	PCB[1].state=STATE_RUNNING;
	PCB[1].tf.cs=USEL(SEG_UCODE(1));
	PCB[1].tf.eip=0x200000;
	PCB[1].tf.error=0;
	PCB[1].tf.ss=USEL(SEG_UDATA(1));
	PCB[1].tf.esp=0x200000+4*MAX_STACK_SIZE;
	PCB[1].tf.eflags=0x202;
	PCB[1].tf.ds=USEL(SEG_UDATA(1));
	PCB[1].tf.es=USEL(SEG_UDATA(1));
	PCB[1].tf.fs=USEL(SEG_UDATA(1));
	PCB[1].tf.gs=USEL(SEG_UDATA(1));

	//init_idle
	PCB[0].pid=0;
	PCB[0].sleepTime=-1;
	PCB[0].timeCount=1;
	PCB[0].state=STATE_RUNNABLE;
	PCB[0].tf.cs=KSEL(SEG_KCODE);
	PCB[0].tf.eip=(uint32_t)idle;
	PCB[0].tf.error=0;
	PCB[0].tf.ss=KSEL(SEG_KDATA);
	PCB[0].tf.eflags=0x202;
	PCB[0].tf.ds=KSEL(SEG_KDATA);
	PCB[0].tf.es=KSEL(SEG_KDATA);
	PCB[0].tf.fs=KSEL(SEG_KDATA);
	PCB[0].tf.gs=KSEL(SEG_KDATA);
}

void initSeg() {
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	gdt[SEG_VIDEO] = SEG(STA_W,         0xb8000,       0xffffffff, DPL_KERN);
	
	for(int i=1;i<MAX_PCB_NUM;i++){
		gdt[SEG_UDATA(i)] = SEG(STA_W,      (i-1)*4*MAX_STACK_SIZE,       0xffffffff, DPL_USER);
		gdt[SEG_UCODE(i)] = SEG(STA_X | STA_R,      (i-1)*4*MAX_STACK_SIZE,       0xffffffff, DPL_USER);
	}
	setGdt(gdt, sizeof(gdt));

	/*
	 * 初始化TSS
	 */
	tss.ss0=KSEL(SEG_KDATA);
	tss.esp0=(uint32_t)&PCB[1].state;
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw %%ax, %%ds;"::"a"(KSEL(SEG_KDATA)));
	asm volatile("movw %%dx, %%ss;"::"d"(KSEL(SEG_KDATA)));
	lLdt(0);
	
}


void clr_screen(){
	asm volatile ("movw %%ax, %%gs"::"a"KSEL(SEG_VIDEO));
	for(int i=0;i<4000;i++)
		printc('\0');
	vp=0;
}

void enterUserSpace(uint32_t entry) {
	initSemaphore();
	clr_screen();
	uint32_t tmp = (uint32_t)&PCB[1].tf.gs;
	asm volatile("movl %0, %%esp"::"r"(tmp));
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $4, %esp");
	asm volatile("addl $4, %esp");
	asm volatile("iret");
}

void loadUMain(void) {	
	/*加载用户程序至内存*/
	struct ELFHeader *elf=(void *)0x300000;
	struct ProgramHeader *ph,*eph;
	for(int i=201;i<=400;i++){
		readSect((void *)(0x300000+(i-201)*512),i);
	}
	ph = (void *)elf + elf->phoff;
	eph = ph + elf->phnum;
	for(;ph < eph;ph++){
		if(ph->type == 1){
			for(int i=0;i<ph->filesz;i++)
				*(unsigned char *)(ph->vaddr+i) = *(unsigned char*)(0x300000+ph->off+i);
		}
	}
	
	enterUserSpace(elf->entry);
}
