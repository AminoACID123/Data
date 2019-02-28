#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string.h>
#include <assert.h>



#include <stdlib.h>  
#include <string.h>  

 
#include <arpa/inet.h>  
#include <net/ethernet.h>  
#include <netpacket/packet.h>  
#include <net/if.h>  
#include <sys/ioctl.h>
  
#define BUFFER_MAX 2048
#define MAX_DEVICE 1
#define MAX_ARP_SIZE 5
#define MAX_ROUTE_INFO 2

unsigned char IP_BROADCAST[4]={255,255,255,255};
unsigned char MAC_NULL[ETH_ALEN]= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  ;
unsigned char MAC_BROADCAST[ETH_ALEN]= {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
char mac_addr1[6]={0x00,0x0c,0x29,0x37,0x54,0x2e};
char mac_addr2[6]={0x00,0x0c,0x29,0x64,0x45,0x0f };


int arp_pointer = 0;

struct device_item{
	int index;
	char ip_addr[4];
	char mac_addr[ETH_ALEN];
}device[MAX_DEVICE];


struct arp_table_item{
	char ip_addr[4];
	char mac_addr[ETH_ALEN];
}arp_table[MAX_ARP_SIZE];


struct route_item{
	char destination[4];
	char gateway[4];
	char netmask[4];
	char mac_addr[6];
	char interface;
}route_info[MAX_ROUTE_INFO];

  

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
	device[0].ip_addr[0]=192; device[0].ip_addr[1]=168; device[0].ip_addr[2]=3; device[0].ip_addr[3]=2;

	device[1].index=1;
	get_mac_addr(device[1].mac_addr,1);
	
	device[1].ip_addr[0]=192; device[1].ip_addr[1]=168; device[1].ip_addr[2]=4; device[1].ip_addr[3]=1;
}

void initArpTable(){
	memset(arp_table,0,sizeof(struct arp_table_item)*MAX_ARP_SIZE);
}
void initRouteInfo(){

	memset(route_info,0,sizeof(struct route_item)*MAX_ROUTE_INFO);
	route_info[0].destination[0]=0;
	route_info[0].destination[1]=0;
	route_info[0].destination[2]=0;
	route_info[0].destination[3]=0;


	route_info[0].gateway[0]=192;
	route_info[0].gateway[1]=168;
	route_info[0].gateway[2]=3;
	route_info[0].gateway[3]=1;

	route_info[0].netmask[0]=255;
	route_info[0].netmask[1]=255;
	route_info[0].netmask[2]=255;
	route_info[0].netmask[3]=0;

	route_info[0].interface=2;
	memcpy(route_info[0].mac_addr,mac_addr1,6);

	route_info[1].destination[0]=192;
	route_info[1].destination[1]=168;
	route_info[1].destination[2]=4;
	route_info[1].destination[3]=0;


	route_info[1].gateway[0]=192;
	route_info[1].gateway[1]=168;
	route_info[1].gateway[2]=4;
	route_info[1].gateway[3]=2;

	route_info[1].netmask[0]=255;
	route_info[1].netmask[1]=255;
	route_info[1].netmask[2]=255;
	route_info[1].netmask[3]=0;
	
	memcpy(route_info[1].mac_addr,mac_addr2,6);
	route_info[1].interface=3;

}







void Routing(char *buffer, char *ip_src, char *ip_dest){

	int i=1;
	int flag = 0;
	for(;i<MAX_ROUTE_INFO;i++){
		if(((ip_dest[0] & route_info[i].netmask[0]) == route_info[i].destination[0]) &&
		   (ip_dest[1] & route_info[i].netmask[1]) == route_info[i].destination[1] &&
		   (ip_dest[2] & route_info[i].netmask[2]) == route_info[i].destination[2] &&
		   (ip_dest[3] & route_info[i].netmask[3]) == route_info[i].destination[3] ){
			flag = 1;
			break;
		}
	}
	if(flag==0) i = 0;
	
	int sockfd= socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	if(sockfd ==-1)
	{
		perror("socket error");
		exit(1);
	}
	struct sockaddr_ll dest_addr;
	dest_addr.sll_family = AF_PACKET;
	dest_addr.sll_protocol = htons(ETH_P_IP);
	dest_addr.sll_halen = ETH_ALEN;
	dest_addr.sll_ifindex = route_info[i].interface;
	memcpy(&dest_addr.sll_addr,route_info[i].mac_addr,6);

        if(sendto(sockfd,buffer,20+64,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0)
        {
		perror("sendto error");
            	return ;
	}
	
}


int main(int argc, char* argv[]){
	int sock_fd;
	int proto;
	int type;
	int n_read;
	char buffer[BUFFER_MAX];

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
	
	
	if((sock_fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP)))<0){
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
			
			if(proto==IPPROTO_ICMP && addr.sll_pkttype==PACKET_HOST && strncmp(ip,ip+4,4)!=0)
			{
				printf("%02hhx.%02hhx.%02hhx.%02hhx ==> %02hhx.%02hhx.%02hhx.%02hhx\n",ip[0],ip[1],ip[2],ip[3],ip[4],ip[5],ip[6],ip[7]);
				printf("ICMP\n",proto);Routing(buffer+14,ip,ip+4);
			}
			
		}
	}

	return -1;
}
