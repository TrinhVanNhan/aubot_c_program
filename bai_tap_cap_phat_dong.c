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
}

int main()
{
	frame_t frame1 ;
	uint8_t *ptt = NULL;
	uint8_t dulieu[8] = {0x98,0x05,0x12,0x01,0x06,0xAC,0x04,0x99};
	copy_data(dulieu,ptt,frame1);
	return 0;
}
