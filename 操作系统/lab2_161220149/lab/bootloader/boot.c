#include "boot.h"

#define SECTSIZE 512

void bootMain(void) {
	/* 加载内核至内存，并跳转执行 */
	struct ELFHeader *elf=(void *)0x200000;
	struct ProgramHeader *ph,*eph;
	for(int i=1;i<=200;i++){
		readSect((void *)(0x200000+(i-1)*512),i);
	}
	ph = (void *)elf + elf->phoff;
	eph = ph + elf->phnum;
	for(;ph < eph;ph++){
		if(ph->type == 1){
			for(int i=0;i<ph->filesz;i++)
				*(unsigned char *)(ph->vaddr+i) = *(unsigned char*)(0x200000+ph->off+i);
			for(int i=ph->filesz;i<ph->memsz;i++)
				*(unsigned char *)(ph->vaddr+i) = 0;
		}
	}
	void (*entry)(void);
	entry=(void *)elf->entry;
	entry();

}

//void waitDisk(void) { // waiting for disk
//	while((inByte(0x1F7) & 0xC0) != 0x40);
//}

void readSect(void *dst, int offset) { // reading a sector of disk
	int i;
//	waitDisk();
	while((inByte(0x1F7) & 0xC0) != 0x40);
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);
	while((inByte(0x1F7) & 0xC0) != 0x40);
//	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}
