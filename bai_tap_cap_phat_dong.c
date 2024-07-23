#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<math.h>
typedef struct {
	uint8_t starbyte;
	uint8_t stopbyte;
	uint8_t len;
	}frame_t;
uint8_t *cap_phat(uint8_t *pt,frame_t frame)
{
	pt = (uint8_t*)(calloc(frame.len +3,sizeof(uint8_t)));
	return pt;
}
void tach_so(char pt[], uint8_t n, uint8_t dll[])
{
	char data[50]="";
	char *ptt = strtok(pt,"|");
	while(ptt)
	{
		strcat(data,ptt);
		ptt = strtok(NULL,"|");
	}
	printf("%s",data);
	printf("\n");
	char *pttt = strtok(data,"0x");
	uint8_t dl[50]={};
	int i=0;
	char *p;
	while(pttt)
	{
		dl[i] = strtol(pttt,&pttt,16);
		pttt = strtok(NULL,"0x");
		i++;
	}
	int x=0;
	for(;x<n;x++)
	{
		dll[x]=dl[x];
	}
}
uint8_t check_sum(uint8_t pt[], uint8_t len)
{
	uint8_t sum = 0;
	int i=0;
	for(;i<len;i++)
	{
		sum = sum + pt[i];
	}
	sum = sum%256;
	return sum;
}
void copy_data(uint8_t pt[], uint8_t *ptt ,frame_t frame)
{
	frame.len = pt[1];
	frame.starbyte = pt[0];
	frame.stopbyte = pt[frame.len +2];
	ptt = cap_phat(ptt,frame);
	int i=0;
	for(;i<frame.len+3;i++)
	{
		ptt[i]=pt[i];
	}
	int j=0;
	for(;j<frame.len+3;j++)
	{
		printf("|%x",ptt[j]);
	}
	printf("\n");
	uint8_t check = check_sum(&ptt[2], frame.len);
	printf("check_sum bang %x",check);
	free(ptt);
}

int main()
{
	frame_t frame1 ;
	uint8_t *ptt = NULL;
	uint8_t dl[50] = "|0x98|0x05|0x12||0x01||0x06||0xAC||0x04||0x99|";
	uint8_t dulieu[]={};
	tach_so(dl,8,dulieu);
	copy_data(dulieu,ptt,frame1);
	return 0;
}
