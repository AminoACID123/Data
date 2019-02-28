#include "lib.h"
#include "types.h"
#define EXIT 1
#define FORK 2
#define WRITE 4
#define SLEEP 5
/*
 * io lib here
 * 库函数写在这
 */
char int_res[20];
char hex_res[20];
char buf[4096];
void strcat(char *dst, char *src,int len){
	int i=0;
	for(;dst[i]!='\0';i++);
	if(len == 0)
		for(int j =0;src[j]!='\0';j++){
			dst[i] = src[j];
			i++;
		}
	else
		for(int j =0;j<len;j++){
			dst[i] = src[j];
			i++;
		}
	dst[i]='\0';
}

void const_strcat(char *dst, const char *src,int len){
	int i=0;
	for(;dst[i]!='\0';i++);
	if(len == 0)
		for(int j =0;src[j]!='\0';j++){
			dst[i] = src[j];
			i++;
		}
	else
		for(int j =0;j<len;j++){
			dst[i] = src[j];
			i++;
		}
	dst[i]='\0';
}



void hextoa(uint32_t input){
  
	for(int i=0;i<20;i++)
		hex_res[i]='\0';
	hex_res[0]='0';
	int i = 0;
	int j = 0;
	while(input >0){
		if((input%16)<=9)
			hex_res[i]='0'+(input%16);
		else
			hex_res[i]='a'+(input%16)-10;
		input = input/16;
		i++;
	}
	i--;
	while(j<i){
		char tmp = hex_res[i];
		hex_res[i]=hex_res[j];
		hex_res[j]=tmp;
		j++;
		i--;
	}
		
}  
void itoa(int input){

	for(int i=0;i<20;i++)
		int_res[i]='\0';
	if(input==0){
		int_res[0]='0';
		return;
	}
	int index=0;
	if(input<0){
		if(input==0x80000000){
			int_res[0]='-';int_res[1]='2';int_res[2]='1';int_res[3]='4';int_res[4]='7';int_res[5]='4';int_res[6]='8';int_res[7]='3';int_res[8]='6';int_res[9]='4';int_res[10]='8';;return;}
		input = -input;
		int_res[0]='-';
		index=1;
	}
	int i=index;
	while(input >0){
		int_res[i]='0'+(input%10);
		input = input/10;
		i++;
	}
	i--;
	while(index<i){
		char tmp = int_res[i];
		int_res[i]=int_res[index];
		int_res[index]=tmp;
		index++;
		i--;
	}
		
}  




int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret = 0;
	asm volatile("pusha");
	/* 内嵌汇编 保存 num, a1, a2, a3, a4, a5 至通用寄存器*/
	asm volatile("pushl %0"::"r"(num));
	asm volatile("pushl %0"::"r"(a1));
	asm volatile("pushl %0"::"r"(a2));
	asm volatile("pushl %0"::"r"(a3));
	asm volatile("pushl %0"::"r"(a4));
	asm volatile("pushl %0"::"r"(a5));

	asm volatile("popl %esi");	
	asm volatile("popl %edi");	
	asm volatile("popl %edx");	
	asm volatile("popl %ecx");	
	asm volatile("popl %ebx");	
	asm volatile("popl %eax");	



	asm volatile("int $0x80");
	
	asm volatile("movl %%eax, %0":"=m"(ret):);

	asm volatile("popa");
	return ret;
}

void sleep(uint32_t time){
	syscall(SLEEP,0,time,0,0,0);
}

int exit(){
	return syscall(EXIT,0,0,0,0,0);
}

int fork(){
	
	int tmp=syscall(FORK,0,0,0,0,0);	
	return tmp;
}

void printf(const char *format,...){
	uint32_t addr = (uint32_t)&format;addr+=4;
	uint32_t start=0;
	uint32_t i=0;
	for(int j=0;j<4096;j++)
		buf[0]='\0';
	while(format[i]!='\0'){
		if(format[i]=='%'){
			if(i!=start){	
				const_strcat(buf,format+start,i-start);
			}
			if(format[i+1]=='s'){
				strcat(buf,(char *)(*(uint32_t *)addr),0);
				addr+=4;
			}
			else if(format[i+1]=='c'){
				char tmp[2];
				tmp[0]=*(uint32_t *)addr;
				tmp[1]='\0';
				strcat(buf,tmp,0);
				addr+=4;
			}
			else if(format[i+1]=='d'){
				itoa(*(int *)addr);
				strcat(buf,int_res,0);
				addr+=4;
			}
			else if(format[i+1]=='x'){
				hextoa(*(uint32_t *)addr);
				strcat(buf,hex_res,0);
				addr+=4;
			}
			start=i+2;
			i+=2;
		}	
		else i++;
	}
	if(start<i){
		const_strcat(buf,format+start,i-start);
	}

	syscall(WRITE,0,(uint32_t)buf,4096,0,0);

}
