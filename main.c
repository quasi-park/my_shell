#include "shell.h"

void lookcd(int argc,char *path[]);
char * getargs(int *argc, char *argv[], char *lbuf, int *input, int *output,int *background, int *pipe, char *inf[],char *outf[]);
extern int checkletter(char bufletter);
extern char * redirectinout(char *lbuf,char *file[],int *flag);
extern void checkpath(int *pathlen, char *paths[], char *sourcepaths);
void checkwait(int sig);
extern void createhistorylist(struct history_list *hlist, int *histfd);
extern void enableRawMode();
extern void disableRawMode();
extern int readline(char *buf, struct history_list *hlist, char *env[], int envlen);

int FOREGROUND;
struct backgroundjobs BACKGROUND_JOBHEAD;

int main(int argc, char *argv[], char *envp[]){
    int agc, i;
    int *returncid;
    int pid;
    int input,output, background;
    struct pipestream pipehead;
    struct pipestream *npipe;
    struct pipestream *ipipe;
    struct history_list hlist, *buflist;
    struct backgroundjobs *bj;
    char lbuf[BUFLEN], *lbuf_pointer;
    char *inf[BUFLEN], *outf[BUFLEN];
    int fd;
    int pipeflag;
    int pfd[2], prevpfd_0, prevpfd_1;
    int pgrpid;
    int fd_group;
    int changed;
    int pathindex;
    int histfd;
    sigset_t disable_intrupt;
    sigset_t disable_sigchld;
    struct sigaction act;

    for(i = 0; envp[i] != NULL;i++ ){
        if(strstr(envp[i],"PATH") != NULL){
            pathindex = i;
            break;
        }
    }

    //get all paths
    int pathlen = 0;
    char *paths[BUFLEN];
    char envp_path[strlen(envp[pathindex]) + 1];
    strcpy(envp_path, envp[pathindex]);
    checkpath(&pathlen, paths, envp_path);

    //initializing vars and structs
    sigemptyset(&disable_intrupt);
    sigaddset(&disable_intrupt, SIGINT);
    sigaddset(&disable_intrupt, SIGTTOU);
    act.sa_handler = &checkwait;
    act.sa_flags = SA_RESTART;

    //initialize history
    hlist.fp = hlist.bp = &hlist;
    hlist.len = 0;

    if(sigaction(SIGCHLD,&act,NULL) < 0){
        perror("sigaction");
    }

    if((fd_group = open("/dev/tty", O_RDWR)) < 0){
        perror("open");
    }
    BACKGROUND_JOBHEAD.bp = BACKGROUND_JOBHEAD.fp = &BACKGROUND_JOBHEAD;
    BACKGROUND_JOBHEAD.jobno = 0;

    createhistorylist(&hlist, &histfd);
    //main shell starts here
    while(1){
        //initialize vars for each loop
        pipehead.bp = pipehead.fp = &pipehead;
        input = output = 0;
        background = 0;
        changed = 0;
        FOREGROUND = 0;
        pipeflag = 1;
        //input
        enableRawMode();
        readline(lbuf, &hlist, paths, pathlen);
        disableRawMode();
        printf("\n");
        lbuf_pointer = lbuf;

        buflist = (struct history_list*)malloc(sizeof(struct history_list));
        if(buflist == NULL){
            fprintf(stderr, "Could not allocate space for history\n");
            exit(1);
        }
        strcpy(buflist->command, lbuf);
        *(strchr(buflist->command,'\n')) = '\0';
        (hlist.bp)->fp = buflist;
        buflist->bp = hlist.bp;
        hlist.bp = buflist;
        buflist->fp = &hlist;
        (hlist.len)++;
        buflist->len = hlist.len;

        write(histfd, lbuf, strlen(lbuf));
        *(strchr(lbuf,'\n')) = '\0';
        //check lbuf and sort.
        while(pipeflag){
            npipe = (struct pipestream*)malloc(sizeof(struct pipestream));
            if(npipe == NULL){
                fprintf(stderr, "Could not allocate space for new pipe stream\n");
                exit(1);
            }
            (pipehead.bp)->fp = npipe;
            npipe->bp = pipehead.bp;
            pipehead.bp = npipe;
            npipe->fp = &pipehead;

            for(i = 0; i < BUFLEN;i++){
                npipe->agv[i] = NULL;
            }
            lbuf_pointer = getargs(&agc,pipehead.bp->agv,lbuf_pointer,&input,&output,&background,&pipeflag, inf,outf);
        }

        if(agc > 0){
            for(ipipe = pipehead.fp; ipipe != &pipehead; ipipe = ipipe->fp){
                //change first input to lowercase
                for(i = 0; i < strlen(ipipe->agv[0]);i++){
                    ipipe->agv[0][i] = tolower(ipipe->agv[0][i]);
                }
                //check input
                if(strcmp(ipipe->agv[0],"exit") == 0){
                    close(histfd);
                    exit(0);
                }else if(strcmp(ipipe->agv[0],"cd") == 0){
                    lookcd(agc,ipipe->agv);
                    pgrpid = getpgrp();
                    changed = 1;
                }else if(strcmp(ipipe->agv[0], "history") == 0){
                    for(buflist = hlist.fp; buflist != &hlist; buflist = buflist->fp){
                        printf("%3d: %s\n", buflist->len, buflist->command);
                    }
                    changed = 1;
                }else{
                    if(ipipe->fp != &pipehead){
                        pipe(pfd);
                        pipeflag = 1;
                    }
                    if((pid =fork()) < 0){
                        fprintf(stderr,"an error occured while forking process.\n");
        	            exit(1);
                    }else if(pid == 0){
                        if(pipeflag){
                            if(ipipe->bp != &pipehead){
                                close(0);
                                dup(prevpfd_0);
                                close(prevpfd_0);
                            }
                            if(ipipe->fp != &pipehead){
                                close(1);
                                dup(pfd[1]);
                            }
                            close(pfd[0]);
                            close(pfd[1]);
                        }
                        if(ipipe->bp == &pipehead && input == 1){
                            fd = open(inf[0], O_RDONLY, 0644);
                            close(0);
                            dup(fd);
                            close(fd);
                        }
                        if(ipipe->fp == &pipehead && output == 1){
                            fd = open(outf[0], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                            close(1);
                            dup(fd);
                            close(fd);
                        }
                        for(i = 0; i < pathlen; i++){
                            char *fullpath = (char *)malloc(sizeof(char) * (strlen(paths[i]) + strlen(ipipe->agv[0]) + 2));
                            if(fullpath == NULL){
                                fprintf(stderr, "Could not allocate space for new pipe stream\n");
                                exit(1);
                            }
                            memset(fullpath, '\0',sizeof(char) * (strlen(paths[i]) + strlen(ipipe->agv[0]) + 2));
                            strcpy(fullpath, paths[i]);
                            // fullpath[strlen(fullpath)] = '\0';
                            *(strchr(fullpath,'\0')) = '/';
                            strcat(fullpath, ipipe->agv[0]);

                            execve(fullpath,ipipe->agv, envp);
                            free(fullpath);
                        }
                        //if none of the path hits, check ipipe->agv itself
                        execve(ipipe->agv[0], ipipe->agv, envp);
                        fprintf(stderr, "mysh: command [%s] was not found.\n",ipipe->agv[0]);
                        exit(0);
                    }else{
                        if(pipeflag){
                            close(pfd[1]);
                            if(ipipe->fp != &pipehead){
                                prevpfd_0 = pfd[0];
                            }else{
                                close(pfd[0]);
                            }
                        }
                            if(ipipe->bp == &pipehead){
                                pgrpid = pid;
                            }
                        if(setpgid(pid,pgrpid) < 0){
                            fprintf(stderr, "%s\n", strerror(errno));
                            exit(10);
                        }
                    }
                }
            }
            if(!changed){
                if(background == 0){
                    if(sigprocmask(SIG_BLOCK,&disable_intrupt,NULL) < 0){
                        fprintf(stderr, "%s\n", strerror(errno));
                    }
                     if((tcsetpgrp(fd_group,pgrpid)) < 0){
                         perror("tcsetpgrp");
                     }
                    for(ipipe = pipehead.fp; ipipe != &pipehead; ipipe = ipipe->fp){
                        FOREGROUND++;
                    }
                    while(FOREGROUND != 0){
                        //this loop waits for all foreground process to end
                    }
                    if(tcsetpgrp(fd_group,getpgrp()) < 0){
                        perror("tcsetpgrp");
                    }
                    if(sigprocmask(SIG_UNBLOCK,&disable_intrupt,NULL) < 0){
                        fprintf(stderr, "%s\n", strerror(errno));
                    }
                }else{
                    bj = (struct backgroundjobs*)malloc(sizeof(struct backgroundjobs));
                    if(bj == NULL){
                        fprintf(stderr, "Could not allocate space for new pipe stream\n");
                        exit(1);
                    }
                    bj->jobno = ((BACKGROUND_JOBHEAD.bp)->jobno) + 1;
                    bj->jobid = pid;
                    (BACKGROUND_JOBHEAD.bp)->fp = bj;
                    bj->bp = BACKGROUND_JOBHEAD.bp;
                    BACKGROUND_JOBHEAD.bp = bj;
                    bj->fp = &BACKGROUND_JOBHEAD;
                    printf("JOBID: [%d]\n",bj->jobno);
                }
            }
            for(ipipe = pipehead.fp; pipehead.fp != &pipehead; ipipe = pipehead.fp){
                pipehead.fp = ipipe->fp;
                (ipipe->fp)->bp = &pipehead;
                free(ipipe);
            }
        }
    }
}


char * getargs(int *argc, char *argv[], char *lbuf, int *input,int *output, int *background,int *pipe, char *inf[],char *outf[]){
  *argc = 0;
  *pipe = 0;
  int firstletter = 1;

  while(1){
      int lettertype;
      if(firstletter){
        while(isblank(*lbuf)){
          lbuf++;
        }
        if(*(lbuf) == '\0'){
          return NULL;
        }
    }

    switch(checkletter(*lbuf)){
        case NORMALLETTERS:
            if(firstletter){
                argv[(*argc)++] = lbuf;
                firstletter = 0;
            }
            break;
        case REDIRECTIN:
            *lbuf = '\0';
            lbuf++;
            lbuf = redirectinout(lbuf,inf,input);
            break;
        case REDIRECTOUT:
            *lbuf = '\0';
            lbuf++;
            lbuf = redirectinout(lbuf,outf,output);
            break;
        case BACKGROUND:
            *background = 1;
            *lbuf = '\0';
            while(*lbuf && !isblank(*lbuf) && (strcmp(lbuf,"\n") != 0)){
                lbuf++;
            }
            break;
        case PIPE:
            *lbuf = '\0';
            *pipe = 1;
            lbuf++;
            return lbuf;
            break;
        default:
            break;
    }

    if(*lbuf == '\0'){
      return NULL;
    }
    if(isblank(*lbuf)){
        firstletter = 1;
        *lbuf = '\0';
    }
    lbuf++;
  }
}

void lookcd(int argc, char *path[]){
    char bufpath[BUFLEN];
    memset(bufpath, '\0', BUFLEN);
    if(argc > 1){
        if(path[1][0] == '~'){
            char *homedir = getenv("HOME");
            strncpy(bufpath,homedir,strlen(homedir));
            strncat(bufpath,path[1]+1,sizeof bufpath - strlen(homedir) - 1);
        }else{
            strncpy(bufpath,path[1],(sizeof bufpath) - 1);
        }
    }else{
        char *homedir = getenv("HOME");
        strncpy(bufpath,homedir,strlen(homedir));
    }
    if(chdir(bufpath) < 0){
        fprintf(stderr, "%s\n", strerror(errno));
    }
}

void checkwait(int sig){
    int returncid;
    struct backgroundjobs *i;
    int bgflag = 0;
    int id;
    if((id = wait(&returncid)) < 0){
        perror("wait");
    }else{
        for(i = BACKGROUND_JOBHEAD.fp; i != &BACKGROUND_JOBHEAD; i = i->fp){
            if((i->jobid) == id){
                (i->bp)->fp = i->fp;
                (i->fp)->bp = i->bp;
                i->fp = NULL;
                i->bp = NULL;
                free(i);
                bgflag = 1;
                break;
            }
        }
        if(!bgflag){
            FOREGROUND--;
        }
    }
}
