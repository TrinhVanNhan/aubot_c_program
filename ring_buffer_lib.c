#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<math.h>
#define SIZE 100
typedef struct {
uint32_t front; // con tro doc
uint32_t rear; // con tro ghi
uint32_t Size;// kich thuoc cua Ring
uint8_t *pt;// con tro den buffer
}BUFFER;
BUFFER t;
uint8_t buffer[SIZE];
int buffer_init(BUFFER *r, uint8_t *buf, uint32_t size);
int check_full(BUFFER *r);
int check_empty(BUFFER *r);
int check_full(BUFFER *r)
{
	if(((r->front == 0)&&(r->rear == r->Size - 1)) || (r->rear +1 == r->front)) // kiem tra co co day khong
	{
		return -1;
	}
	return 0;
}
int check_empty(BUFFER *r) // kiem tra xem co rong khong
{
	if(r->front == -1)
	{
		return -1;
	}
	return 0;
}
int buffer_init(BUFFER *r, uint8_t *buf, uint32_t size)
{
	if(r==NULL || buf == NULL || size <= 2)
	{
		return -1;
	}
	r->front = -1;
	r->rear = -1;
	r->Size = size;
	r->pt = buf;
	return 0;
}
int put_buf(BUFFER *r,uint32_t value) // ham them phan tu cho queue
{
	if(check_full(r)) // check xem co day khong de them phan tu
	{
		printf("Ring full\n");
		return -1;
	}
	if(r->front  == -1) // TH ma chua bo phan tu nao vao queue 
	{
		r->front = 0;
	}
	r->rear = (r->rear + 1)%(r->Size); // thiet lap rear moi trong queue tron
	r->pt[r->rear] = value;
}
uint32_t get_buf(BUFFER *r) // lay phan tu ra khoi queue
{
	if(check_empty(r)) // check xem co rong khong de lay phan tu ra
	{
		printf("Ring rong\n");
		return -1;
	}
	uint32_t element = r->pt[r->front]; // lay data ra
	if(r->front == r->rear) // TH ma da lay het data và queue rong
	{
		r->front = r->rear = -1; //reset lai front và rear ve -1;
	}
	else  // con neu van con data thi cap nhat lai front
	{
		r->front = (r->front +1)%(r->Size);
	}
	return element;
}
void in_Ring(BUFFER *r)// ham in ra queue
{
	if(r->front == -1) // check xem co rong hay khong
	{
		printf("Ring rong\n");
		return;
	}
	int i = r->front;
	for(;i != r->rear; i = (i+1)%(r->Size)) // in thieu gia tri cuoi
	{
		printf("du lieu la %d\n",r->pt[i]);
	}
	printf("du lieu la %d\n",r->pt[r->rear]); // in bu phan tu cuoi cung trong queue // co the in la printf("du lieu la %d\n",items[i]);
}
int main()
{
	buffer_init(&t,buffer,5);
	put_buf(&t,1);
	put_buf(&t,2);
	put_buf(&t,3);
	put_buf(&t,4);
	put_buf(&t,5);
	uint32_t f = get_buf(&t);
	printf("-> %d\n",f);
	put_buf(&t,7);
	uint32_t e = get_buf(&t);
	printf("-> %d\n",e);
	put_buf(&t,8);
	in_Ring(&t);
	uint32_t c = get_buf(&t);
	printf("-> %d\n",c);
	in_Ring(&t);
	return 0;
}
