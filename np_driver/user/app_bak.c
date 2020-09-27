#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "user_pkt_transfer.h"
#include "udriver_usrpart_pkt.h"

// 接收和发送的文件句柄
int fd_snd, fd_rcv;
// 描述符环
// 环大小为2048
struct desc_ring *dr;
// 收发线程
pthread_t *s_thrd, *r_thrd;

// 初始化描述符环
// 环大小为DESC_NUM2048
struct desc* np_init_desc_ring(void){
	struct desc *head, *current, *next;
	int i;
//	printf("the size of struct desc is %lu\n", sizeof(struct desc));
	head = (struct desc *)malloc(sizeof(struct desc));
	current = head;
	memset(head, 0, sizeof(struct desc));
	for(i = 0; i < DESC_NUM; i ++ ){
		next = (struct desc *)malloc(sizeof(struct desc));
		memset(next, 0, sizeof(struct desc));
		if(i < DESC_NUM-1){
			current->nxt = next;
		}else if(i == DESC_NUM-1)
		{
			current->nxt = head;
		}
		current = current->nxt;	
	}
	return head;
}

// write - send
// 0:success
// 文件描述符是全局变量
int np_write(char *user_ptr){
	int err;
	
	if(fd_snd < 0){
		printf("fail to open the send file\n");
		return -1;
	}
	err = write(fd_snd, user_ptr, 2048);
	if(err != 0){
		printf("fail to send pkt\n");
	}
	return err;
}

// read - recv
// 阻塞式读
// 0：接收成功
// -1：用户地址不合法
// -2：拷贝失败
// -3：缓冲区内无报文
// 内核中的read函数不是阻塞式的
// 当当前轮询的缓冲区无报文时，会立即返回
// 内核中的全局接收指针不会移动
// 文件描述符是全局变量
int np_read(char *user_ptr){
	int err;
		
	if(fd_rcv < 0){
		printf("fail to open the recv file\n");
		return -1;
	}
//	printf("start read ...\n");
	err = read(fd_rcv, user_ptr, 2048);
	while(err != 0){
		err = read(fd_rcv, user_ptr, 2048);
	}
	return err;
}

// 发送报文的线程所执行的函数
// struct desc_ring是一个由发送和接收线程共同访问的结构体
// 在主函数中声明为全局变量
void np_send(struct desc_ring *dr){
	int err;
	struct desc *snd;
	
	snd = dr->snd;
	while(1){
		// 禁止同时读写某片内存
		if(dr->snd != dr->rcv){
			// 该描述符上有报文
			if(snd->busy == 1){
				// 发送报文
				err = np_write((char *)(snd->pkt));
				// 释放pkt内存
				free((void *)snd->pkt);
				snd->pkt = 0;
				// 释放该描述符
				snd->busy = 0;
				// 发送指针后移
				snd = snd->nxt;
				dr->snd = snd;
			}
		}
	}
}

// 接收报文的线程所执行的函数
// 从网卡接收上来的报文挂在描述符环上
// struct desc_ring是一个由发送和接收线程共同访问的结构体
// 在主函数中声明为全局变量
void np_recv(struct desc_ring *dr){
	int err;
	struct cp_packet *pkt;
	struct desc *rcv;
	
	printf("this this the np_recv func\n");
	rcv = dr->rcv;
	while(1){
		printf("this this the np_recv func\n");
		// 禁止同时读写某片内存
		if(dr->rcv != dr->snd){
			// 该描述符是否空闲
			// 若空闲
			if(rcv->busy == 0){
				pkt = malloc(PKT_SIZE);
				memset(pkt, 0, PKT_SIZE);
				// 接收报文
				err = np_read((char *)pkt);
				// 接收成功
				if(err == 0){
					// 把报文挂载描述符环上
					rcv->pkt = pkt;
					rcv->busy = 1;
					// 接收指针后移
					rcv = rcv->nxt;
					dr->rcv = rcv;
				}
				// 接收失败
				else{
					free((void *)pkt);
					pkt = 0;
				}
			}
		}
	}
}

// 创建接收和发送线程
// 将收发线程通过传进来的参数返回给调用函数
int create_transfer_thread(pthread_t *s_thrd, pthread_t *r_thrd, 
							struct desc_ring *dr){
	int err;
	printf("before init the thread\n");
	s_thrd = (pthread_t *)malloc(sizeof(pthread_t));
	r_thrd = (pthread_t *)malloc(sizeof(pthread_t));
	memset(s_thrd, 0, sizeof(pthread_t));
	memset(r_thrd, 0, sizeof(pthread_t));
	printf("after init the thread\n");
	// 先创建接收线程
	err = pthread_create(r_thrd, NULL, 
							(void *)&np_recv, 
							(void *)dr);
	if(err){
		printf("fail to create recv thread\n");
		goto err_thread_create;
	}
	// 再创建发送线程
	err = pthread_create(s_thrd, NULL, 
							(void *)&np_send, 
							(void *)dr);
	if(err){
		printf("fail to create send thread\n");
		goto err_thread_create;
	}
	printf("creating the thread successfully\n");
	return 0;
err_thread_create:
	return -1;
}

void handler(int sig){
	if(fd_snd)
		close(fd_snd);
	if(fd_rcv)
		close(fd_rcv);
	printf("%s\n", "transferring is over!!!");
	exit(0);
}

int main(){
	int err;
	
//	printf("size of cp_packet %d\n", sizeof(struct cp_packet));
//	struct cp_packet *pkt = malloc(PKT_SIZE);
	
	// 打开收发文件
	fd_snd = open(NP_PCI_CDEV_FILE_PATH_send, O_RDWR);
	fd_rcv = open(NP_PCI_CDEV_FILE_PATH_recv, O_RDWR);

	// 初始化描述符环
	dr = (struct desc_ring *)malloc(sizeof(struct desc_ring));
	dr->ring = np_init_desc_ring();
	printf("the ring addr is %p\n", dr->ring);
	dr->d_num = DESC_NUM;
	printf("%d\n", dr->d_num);
	dr->snd = dr->ring;
	printf("the send ptr addr is %p\n", dr->snd);
	dr->rcv = dr->ring;
	printf("the recv ptr addr is %p\n", dr->rcv);
	
	printf("before start up the transferring thread\n");
	// 初始化并启动收发线程
	err = create_transfer_thread(s_thrd, r_thrd, dr);
	printf("after start up the transfering thread\n");
	
	signal(SIGINT, handler);
	
	if(s_thrd != NULL && r_thrd != NULL){
		pthread_join(*s_thrd, NULL);
		pthread_join(*r_thrd, NULL);
		printf("%s\n", "polling thread is over");
	}
/**/
/*	
	fd_snd = open(NP_PCI_CDEV_FILE_PATH_send, O_RDWR);
	fd_rcv = open(NP_PCI_CDEV_FILE_PATH_recv, O_RDWR);
	
	
	
//	memset(pkt, 0xf, PKT_SIZE);	
	while(1){
		err_recv = np_recv(fd_recv, (char *)pkt);
		// 成功收到报文
		if(err_recv == 0){
			// show pkt
		//	np_show_pkt((char *)pkt);
			err_send = np_send(fd_send, (char *)pkt);
		}else{
			printf("fail to recv a pkt\n");
		}
	}
//	memcpy(pkt->pkt_data, tcp_pkt, sizeof(tcp_pkt));
//	err = send((char *)pkt);
//	if(err == 0){
		// show pkt
//		np_show_pkt((char *)pkt);
//	}else{
//		printf("fail to send a pkt\n");
//	}

	signal(SIGINT, handler);
*/	
	printf("the main func is over\n");
	return 0;
}
