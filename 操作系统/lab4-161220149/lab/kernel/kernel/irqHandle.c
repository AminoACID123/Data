#include "x86.h"
#include "device.h"

#define SYS_EXIT 1
#define SYS_FORK 2
#define SYS_WRITE 4
#define SYS_SLEEP 5
#define SEM_INIT 6
#define SEM_POST 7
#define SEM_WAIT 8



uint32_t vp= 0;

int current=1;

void syscallHandle(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

int puts(struct TrapFrame *tf);

void printc(unsigned char ch);

void TimerInterrupt(struct TrapFrame *tf);

void schedule();

uint32_t addr_u2k(int id,uint32_t addr);

void run(uint32_t);

int sem_post(struct TrapFrame *tf);


int sem_init(struct TrapFrame *tf){
	int i=0;
	for(;Sem[i].used!=0;i++);
	if(i>=10)
		return -1;
	Sem[i].used=1;
	Sem[i].value=tf->ecx;
	for(int j=0;j<5;j++)
		Sem[i].list[j]=-1;
	return i;
}


int sem_post(struct TrapFrame *tf){
	if(tf->ecx>=10 || tf->ecx<0)return -1;
	if(Sem[tf->ecx].used==0)return -1;
	
	struct Semaphore *sem=&Sem[tf->ecx];
	sem->value++;
	int i=0;
	if(sem->value<=0){
		for(;sem->list[i]==-1 && i<5;i++);
		if(i<5){
			PCB[sem->list[i]].state=STATE_RUNNABLE;
			PCB[sem->list[i]].timeCount=1;	
			sem->list[i]=-1;
		}
	}
	return 0;
}


int sem_wait(struct TrapFrame *tf){
	if(tf->ecx>=10 || tf->ecx<0)return -1;
	if(Sem[tf->ecx].used==0)return -1;

	struct Semaphore *sem=&Sem[tf->ecx];
	sem->value--;
	if(sem->value<0){
		sem->list[current]=current;
		PCB[current].timeCount=-1;
		PCB[current].state=STATE_BLOCKED;
		schedule();
	}
	return 0;
}


int sem_destroy(struct TrapFrame *tf){
	if(tf->ecx>=10 || tf->ecx<0)return -1;
	if(Sem[tf->ecx].used==0)return -1;

	Sem[tf->ecx].used=0;
	return 0;
}

uint32_t addr_u2k(int id,uint32_t addr){
	
	uint32_t paddr=addr+(id-1)*4*MAX_STACK_SIZE;
	return paddr;
}

void schedule(){
	uint32_t index=0;
	for(int i=0;i<MAX_PCB_NUM;i++){
		if(PCB[i].state==STATE_BLOCKED && PCB[i].sleepTime<0 && PCB[i].timeCount!=-1)
			PCB[i].state=STATE_RUNNABLE;
	}
	for(int i=1;i<MAX_PCB_NUM;i++){
		if(PCB[i].state==STATE_RUNNING){
			index=i;break;
		}
		if(PCB[i].state==STATE_RUNNABLE){
			index=i;break;
		}
	}
	run(index);	
}

void printc(unsigned char ch){
	asm volatile ("movw %%ax, %%gs"::"a"KSEL(SEG_VIDEO));
	if(ch=='\n'){
		vp = (vp/160+1)*160;
		return;
	}
	unsigned short tmp = ch+0x0c00;
	uint32_t addr = 0xb8000+vp;
	*(unsigned short *)addr=tmp;
//	asm volatile ("movl %%ebx, %%edi"::"b"(vp));
//	asm volatile ("movw %0, %%ax"::"a"(tmp));
	//asm volatile ("movw %ax, %gs:(%edi)");
	vp+=2;
}
 
void idle(){
	uint32_t tmp=((uint32_t)&PCB[0].tf.esp);
	asm volatile("movl %0, %%esp"::"r"(tmp));
	while(1)
		waitForInterrupt();
}

void run(uint32_t id){
	
		current=id;
		PCB[id].state=STATE_RUNNING;
		tss.ss0=KSEL(SEG_KDATA);
		tss.esp0=((uint32_t)&PCB[id].state);
		uint32_t tmp = (uint32_t)&PCB[id].tf.gs;
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
	

void TimerInterrupt(struct TrapFrame *tf){
	for(int i=1;i<MAX_PCB_NUM;i++)
		if(PCB[i].state==STATE_BLOCKED)
			PCB[i].sleepTime--;
	schedule();
}


void irqHandle(struct TrapFrame *tf) {
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax,%%ds"::"a"(KSEL(SEG_KDATA)));
	switch(tf->irq) {
	
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
		case 0x20:
			TimerInterrupt(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

int sys_write(struct TrapFrame *tf){
	
	int len=0;
	uint32_t addr=addr_u2k(current,tf->ecx);
	for(int i=0;i<tf->edx;i++){
		if(*(char *)(addr+i)!='\0'){
			
			printc(*(char *)(addr+i));
			
			len++;
		}
		else break;

	}
	return len;
}

int sys_exit(struct TrapFrame *tf){
	PCB[current].state=STATE_DEAD;
	schedule();
	return 0;
}

int sys_fork(struct TrapFrame *tf){
	
	uint32_t child=2;
	for(;PCB[child].state!=STATE_DEAD;child++);
	PCB[child].pid=child;
	PCB[child]=PCB[current];
	PCB[child].state=STATE_RUNNABLE;
	PCB[child].tf.eax=0;
	PCB[child].tf.ss=USEL(SEG_UDATA(child));
	PCB[child].tf.ds=USEL(SEG_UDATA(child));
	PCB[child].tf.cs=USEL(SEG_UCODE(child));
	uint32_t addr2=0x200000+(current-1)*4*MAX_STACK_SIZE;
	uint32_t addr1=(0x200000+(child-1)*4*MAX_STACK_SIZE);
	for(int i=0;i<4*MAX_STACK_SIZE;i++)
		*(char *)(addr1+i)=*(char *)(addr2+i);
	PCB[child].tf.eax=0;
	return child;
}



int sys_sleep(struct TrapFrame *tf){
	
	PCB[current].sleepTime=tf->ecx;
	PCB[current].state=STATE_BLOCKED;
	current=-1;
	schedule();
	return 0;	
}

void syscallHandle(struct TrapFrame *tf) {
	/* 实现系统调用*/
	if(tf->eax==SYS_WRITE){
		
		tf->eax = sys_write(tf);}
	else if(tf->eax==SYS_FORK)
		tf->eax = sys_fork(tf);
	else if(tf->eax==SYS_SLEEP)
		tf->eax = sys_sleep(tf);
	else if(tf->eax==SYS_EXIT)
		tf->eax = sys_exit(tf); 
	else if(tf->eax==SEM_INIT)
		tf->eax = sem_init(tf); 
	else if(tf->eax==SEM_POST)
		tf->eax = sem_post(tf);  
	else if(tf->eax==SEM_WAIT)
		tf->eax = sem_wait(tf);    
}



void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
