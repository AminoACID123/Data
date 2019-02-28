.code32

.global irqEmpty
irqEmpty:
	pushl $0 // push dummy error code
	pushl $-1 // push interrupt vector into kernel
	jmp asmDoIrq

.global irqGProtectFault
irqGProtectFault:
	pushl $0xd
	jmp asmDoIrq

.global irqSyscall
irqSyscall:
	pushl $0 // push dummy error code
	pushl $0x80 // push interrupt vector into kernel stack
	jmp asmDoIrq

.global vector0   
vector0:   pushl $0;  pushl    $0; jmp asmDoIrq
.global vector1    
vector1:   pushl $0;  pushl    $1; jmp asmDoIrq
.global vector2    
vector2:   pushl $0;  pushl    $2; jmp asmDoIrq
.global vector3    
vector3:   pushl $0;  pushl    $3; jmp asmDoIrq
.global vector4    
vector4:   pushl $0;  pushl    $4; jmp asmDoIrq
.global vector5    
vector5:   pushl $0;  pushl    $5; jmp asmDoIrq
.global vector6    
vector6:   pushl $0;  pushl    $6; jmp asmDoIrq
.global vector7    
vector7:   pushl $0;  pushl    $7; jmp asmDoIrq
.global vector8    
vector8:              pushl    $8; jmp asmDoIrq
.global vector9    
vector9:   pushl $0;  pushl    $9; jmp asmDoIrq
.global vector10   
vector10:             pushl   $10; jmp asmDoIrq
.global vector11   
vector11:             pushl   $11; jmp asmDoIrq
.global vector12   
vector12:             pushl   $12; jmp asmDoIrq
.global vector14   
vector14:             pushl   $14; jmp asmDoIrq
.global vector16   
vector16:  pushl $0;  pushl   $14; jmp asmDoIrq


.global asmDoIrq
asmDoIrq:
	pushal // push process state into kernel stack
	pushl %esp
	call irqHandle
	addl $4, %esp
	popal
	addl $4, %esp //interrupt vector is on top of kernel stack
	addl $4, %esp //error code is on top of kernel stack
	iret
