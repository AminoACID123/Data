#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  

#include <arpa/inet.h>  
#include <net/ethernet.h>  
#include <netpacket/packet.h>  
#include <net/if.h>  
#include <sys/ioctl.h>
#include <time.h>
  

#include <arpa/inet.h>


#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

//struct sockaddr_in dest_addr;
struct sockaddr_in from;
struct timeval start;
struct timeval end;
struct timeval trecv;
struct timeval tsend;
char sendpacket[2048]={0x45,0,0,0x54,0,0,0,0,0x40,1,0,0};
char mac_addr[6]={0x00,0x0c,0x29,0xbf,0xc2,0x15};

int arp_pointer=0;
unsigned char MAC_NULL[ETH_ALEN]= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  ;
unsigned char MAC_SOURCE[ETH_ALEN]= {0x00, 0x0c, 0x29, 0xa8, 0x64, 0xcc}  ;
//冒充的IP  
#define IP_TRICK "192.168.1.1"  
//目标机器的MAC  
unsigned char MAC_TARGET[ETH_ALEN]= {0x00, 0x0c, 0x29, 0x37, 0x54, 0x24}; 
unsigned char MAC_BROADCAST[ETH_ALEN]= {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};   
//目标机器的IP  
//#define IP_TARGET "192.168.1.64"  
#define MAX_DEVICE 1
#define MAX_ARP_SIZE 5
#define MAX_ROUTE_INFO 5

struct device_item{
	int index;
	char ip_addr[4];
	char mac_addr[ETH_ALEN];
};
struct device_item device[MAX_DEVICE];


struct arp_table_item{
	char ip_addr[4];
	char mac_addr[ETH_ALEN];
};
struct arp_table_item arp_table[MAX_ARP_SIZE];


struct route_item{
	char destination[4];
	char gateway[4];
	char netmask[4];
	char mac_addr[6];
	char interface;
};
struct route_item route_info[MAX_ROUTE_INFO];

void die(const char *pre);  
  

void get_mac_addr(char *mac_addr,int index){
	const char *if_name0="eth0";
	const char *if_name1="eth1";
	struct ifreq req;

	memset(&req, 0, sizeof(req));
	if(index==0)
		strncpy(req.ifr_name, if_name0, IFNAMSIZ - 1);
	else
		strncpy(req.ifr_name, if_name1, IFNAMSIZ - 1);
	int sockfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	ioctl(sockfd, SIOCGIFHWADDR, &req);
	memcpy(mac_addr, req.ifr_hwaddr.sa_data, ETH_ALEN);
	
}



void initDevice(){
	device[0].index=0;
	get_mac_addr(device[0].mac_addr,0);
	device[0].ip_addr[0]=192; device[0].ip_addr[1]=168; device[0].ip_addr[2]=4; device[0].ip_addr[3]=2;
}

void initArpTable(){
	memset(arp_table,0,sizeof(struct arp_table_item)*MAX_ARP_SIZE);
}
void initRouteInfo(){

	memset(route_info,0,sizeof(struct route_item)*MAX_ROUTE_INFO);
	memset(route_info[0].destination,0,4);
	route_info[0].gateway[0]=192;
	route_info[0].gateway[1]=168;
	route_info[0].gateway[2]=4;
	route_info[0].gateway[3]=1;

	route_info[0].netmask[0]=255;
	route_info[0].netmask[1]=255;
	route_info[0].netmask[2]=255;
	route_info[0].netmask[3]=0;

	route_info[0].interface=0;
	memcpy(route_info[0].mac_addr,mac_addr,6);

}

 

unsigned short cal_chksum(unsigned short *addr,int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short check_sum = 0;
 
    while(nleft>1)       //ICMP包头以字（2字节）为单位累加
    {
        sum += *w++;
        nleft -= 2;
    }
 
    if(nleft == 1)      //ICMP为奇数字节时，转换最后一个字节，继续累加
    {
        *(unsigned char *)(&check_sum) = *(unsigned char *)w;
        sum += check_sum;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    check_sum = ~sum;   //取反得到校验和
    return check_sum;
}
 

int pack(char *buffer)
{
    int i,packsize;
	
    struct icmp *icmp;
    struct timeval *tval;
	char *tmp[4];
	memcpy(tmp,buffer+26,4);
	memcpy(buffer+26,buffer+30,4);
	memcpy(buffer+30,tmp,4);
	buffer[25]=0;
	buffer[24]=0;
	unsigned short ip_chksum = cal_chksum((unsigned short *)(buffer+14),20);
	//printf("%04hhx\n",ip_chksum);
	buffer[25] = ip_chksum>>8;
	buffer[24] = ip_chksum&0xff;
    icmp = (struct icmp*)(buffer+34);
    icmp->icmp_type = ICMP_ECHOREPLY; //ICMP_ECHO类型的类型号为0
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = 0;    //发送的数据报编号

 	
    packsize = 8 + 56;     //数据报大小为64字节
    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);        //记录发送时间
    //校验算法
    icmp->icmp_cksum =  cal_chksum((unsigned short *)icmp,packsize); 
    return packsize;
} 

int send_icmp_reply(char *buffer){
	int sockfd= socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	if(sockfd ==-1)
	{
		perror("socket error");
		exit(1);
	}
	int packetsize;
	packetsize = pack(buffer);   //设置ICMP报头

	struct sockaddr_ll dest_addr;
	dest_addr.sll_family = AF_PACKET;
	dest_addr.sll_protocol = htons(ETH_P_IP);
	dest_addr.sll_halen = ETH_ALEN;
	dest_addr.sll_ifindex = 2;
	memcpy(&dest_addr.sll_addr,route_info[0].mac_addr,6);

        if(sendto(sockfd,buffer+14,64+20,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0)
        {
		perror("sendto error");
            	return -1;
	}
	

}


int main(int argc, char* argv[]){
	int sock_fd;
	int proto;
	int type;
	int n_read;
	char buffer[2048];

	char *eth_head;
	char *ip_head;
	char *arp_head;
	char *tcp_head;
	char *udp_head;
	char *icmp_head;
	unsigned char *p;
	initDevice();
	initRouteInfo();
	initArpTable();
	struct sockaddr_ll addr;
	socklen_t addr_len = sizeof(addr);
	
	
	if((sock_fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))<0){
		printf("error create raw socket\n");
		return -1;
	}

	while(1){
		n_read = recvfrom(sock_fd,buffer,2048,0,(struct sockaddr *)&addr,&addr_len);
		if(n_read < 42){
			printf("error when recv msg\n");
			return -1;
		}


		eth_head = buffer;
		type = ((eth_head+12)[0]<<8)+(eth_head+13)[0];
		p= eth_head;
		if (type ==0x0800){
			ip_head = eth_head+14;
			char *ip = ip_head+12;
			
						
			proto = (ip_head +9)[0];
			p = ip_head + 12;
			
			if(proto==IPPROTO_ICMP && addr.sll_pkttype==PACKET_HOST && strncmp(ip+4,device[0].ip_addr,4)==0)
			{
				send_icmp_reply(buffer);
				
				
			}
			
		}
	}

	return -1;
}

