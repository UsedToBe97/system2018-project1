#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int long_size = sizeof(long);//字长 若本机是64位，则字长应为8字节long

/**
* TODO 反转str指针指向的字符串
**/
void reverse(char *str){
	int i, j; 
	char ch;
	for (i = 0, j = strlen(str) - 2; i <= j; ++i, --j) {
		ch = str[i];
		str[i] = str[j];
		str[j] = ch;
	}
}

/**
* TODO 从子进程读数据, 也就是从首地址addr中读取len个字节，并写到str中。不要忘了加'\0'
* 使用ptrace的 PTRACE_PEEKDATA 来读，需要注意的是由于64位机器的字长是8byte，
* 所以ptrace每次读取的长度都是8byte。
* 你可能会用到：memcpy函数
**/
void getdata(pid_t child, long addr, char *str, int len){
	int i = 0, j = len / 8;
	union u {
		long v;
		char ch[8];
	} res;
	char *laddr = str;
	while (i < j) {
		res.v = ptrace(PTRACE_PEEKDATA, child, addr + 8 * i, NULL);
		memcpy(laddr, res.ch, 8);
		++i;
		laddr += 8;
	}
	j = len % 8;
	if (j) {
		res.v = ptrace(PTRACE_PEEKDATA, child, addr + 8 * i, NULL);
		memcpy(laddr, res.ch, j);
	} 
	str[len] = '\0';
}

/**
 * TODO 往子进程写数据 从str中得到需要写的长度为len的字符串，写到地址addr中去。
 * 使用 ptrace 的 PTRACE_POKEDATA 来写，需要注意的是由于64位机器的字长是8byte。
 * */
void putdata(pid_t child, long addr, char *str, int len){
	int i = 0, j = len / 8;
	union u {
		long v;
		char ch[8];
	} res;
	char *laddr = str;
	while (i < j) {
		memcpy(res.ch, laddr, 8);
		ptrace(PTRACE_POKEDATA, child, addr + 8 * i, res.v);
		++i;
		laddr += 8;
	}
	j = len % 8;
	if (j) {
		memcpy(res.ch, laddr, j);	
		ptrace(PTRACE_POKEDATA, child, addr + 8 * i, res.v);
	} 
}

int main(){
  pid_t child;
  child = fork();
  if (child < 0) {
    printf("fork error");
  } else if(child == 0) {
     //子进程执行
     ptrace(PTRACE_TRACEME, 0, NULL, NULL);
     execl("fantasy", "fantasy", NULL);
  } else {
    //父进程执行
    long orig_rax;
    long params[3];
    int status;
    char *str, *laddr;
    int toggle = 0;
    while(1) {
      //等待子进程信号
      wait(&status);
      if(WIFEXITED(status)) //遇到子进程退出信号，退出循环
          break;
      // 使用 PTRACE_PEEKUSER 来获取系统调用号。
      orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, NULL);
      if(orig_rax == SYS_write) {
        if(toggle == 0) {
          toggle = 1;
          /**
          * TODO 使用 PTRACE_PEEKUSER 来获取 RDI, RSI, RDX寄存器的值，
          * 分别存到params[0], params[1], params[2]中。
          * 需要注意的是，RAX的宏定义是 ORIG_RAX，
          * 而 RDI RSI RDX 的宏定义为 RDI RSI RDX
          **/
          
          params[0] = ptrace(PTRACE_PEEKUSER, child, 8 * RDI, NULL);
          params[1] = ptrace(PTRACE_PEEKUSER, child, 8 * RSI, NULL);
          params[2] = ptrace(PTRACE_PEEKUSER, child, 8 * RDX, NULL);
          str = (char *)calloc((params[2]+1), sizeof(char));
          getdata(child, params[1], str,
                  params[2]);
          reverse(str);
          putdata(child, params[1], str,
                  params[2]);
         } else {
          toggle = 0;
       }
     }
    /**
    * TODO 使用 PTRACE_SYSCALL 来让子进程进行系统调用。
    **/
    	ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }
  }
  return 0;
}
