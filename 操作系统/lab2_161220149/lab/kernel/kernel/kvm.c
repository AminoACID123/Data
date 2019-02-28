#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];
TSS tss;

#define SECTSIZE 512

extern void printc(char);
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

void initSeg() {
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	gdt[SEG_VIDEO] = SEG(STA_W,         0xb8000,       0xffffffff, DPL_KERN);
	setGdt(gdt, sizeof(gdt));

	/*
	 * 初始化TSS
	 */
	tss.ss0=KSEL(SEG_KDATA);
	tss.esp0=0x1ffff0;
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
	/*
	 * Before enter user space 
	 * you should set the right segment registers here
	 * and use 'iret' to jump to ring3
	 */
	clr_screen();
	asm volatile("sti");
	asm volatile("push %0"::"r"(USEL(SEG_UDATA)));
	asm volatile("push %0"::"r"(0x1000000));
	asm volatile("pushfl");
	asm volatile("pushl %%eax"::"a"(USEL(SEG_UCODE)));	
	asm volatile("pushl %%eax"::"a"(entry));
	asm volatile("movw %%ax, %%ds"::"a"(USEL(SEG_UDATA)));
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
