#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<signal.h>
#include<termios.h>
#include<dirent.h>

#define BUFLEN 256

#define NORMALLETTERS 1
#define REDIRECTIN 2
#define REDIRECTOUT 3
#define PIPE 4
#define BACKGROUND 5
#define ENDOFFILE 6
#define ENDOFLINE 7

#define ARROWUP 'A'
#define ARROWDOWN 'B'
#define ARROWRIGHT 'C'
#define ARROWLEFT 'D'

#define CTRL_KEY(k) ((k) & 0x1f)

#define MODEPATH 1
#define MODEDIR 2

//lists
struct pipestream{
    struct pipestream *bp;
    struct pipestream *fp;
    char *agv[BUFLEN];
};

struct backgroundjobs{
    struct backgroundjobs *bp;
    struct backgroundjobs *fp;
    int jobno;
    int jobid;
};

struct history_list{
    struct history_list *bp;
    struct history_list *fp;
    char command[BUFLEN];
    int len;
};

struct bufchars{
    struct bufchars *bp;
    struct bufchars *fp;
    char c;
    int len;
};

struct directory_info{
    struct directory_info *bp;
    struct directory_info *fp;
    char name[BUFLEN];
    int ttl;
};
