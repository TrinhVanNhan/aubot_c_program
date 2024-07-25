#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<math.h>
void tach_frame(char *pt)
{
	uint8_t aray[50]={};
	uint8_t *ptt = pt;
	char *a ;
	int i=0;
	while(ptt!=NULL)
	{
	   aray[i]=	strtol(ptt,&a,16);
	   ptt++;
	   i++;
	}
	int j=0;
	for(;j<i;j++)
	{
		printf("%x\n",aray[j]);
	}
}
int main()
{
	char famre[256] = "0x050x070xAB0xCD0x890x85";
	tach_frame(famre);
	return 0;
}
