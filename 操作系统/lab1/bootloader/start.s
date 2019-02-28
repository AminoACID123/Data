.code16

.global start
start:

.code32
start32:

.p2align 2
gdt:
	.word 0,0 # empty entry
	.byte 0,0,0,0

gdtDesc:
	.word (gdtDesc - gdt -1)
	.long gdt
