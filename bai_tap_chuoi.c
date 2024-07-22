#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
// dung ham atoi(chuoi)
// dung ham strtok de chia chuoi thanh cac chuoi con voi chuong trinh duoc chay
void tach_so( char pt[])
{
	int data[4];
	char *ptt = strtok(pt,".");
	int i=0;
	while(ptt)
	{
		data[i]= atoi(ptt);
		ptt = strtok(NULL,".");
		i++;
	}
	int j=0;
	for(;j<4;j++)
	{
		printf("%d\n",data[j]);
	}
}

int main()
{
	char IP[15]= "192.168.1.10";
	tach_so(IP);
	return 0;

}
