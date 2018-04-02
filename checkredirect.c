#include "shell.h"

int checkletter(char bufletter);
char * redirectinout(char *lbuf,char *file[],int *flag);
void checkpath(int * pathlen, char *paths[], char *sourcepaths);
void createhistorylist(struct history_list *hlist, int *histfd);

int checkletter(char bufletter){
    switch(bufletter){
        case '>':
            return REDIRECTOUT;
            break;
        case '<':
            return REDIRECTIN;
            break;
        case '|':
            return PIPE;
            break;
        case '&':
            return BACKGROUND;
            break;
        case '\0':
            return ENDOFFILE;
        default:
            return NORMALLETTERS;
    }
}

char * redirectinout(char *lbuf,char *file[],int *flag){
    while(isblank(*lbuf)){
        lbuf++;
    }
    if(*(lbuf) == '\0'){
      return lbuf;
    }
    file[0] = lbuf;
    while(!isblank(*lbuf) && (*lbuf != '\0')){
      lbuf++;
    }

    *flag = 1;
    return lbuf;
}

void checkpath(int *pathlen, char *paths[], char *sourcepaths){
    *pathlen = 0;
        while(*sourcepaths != '='){
            sourcepaths++;
        }

        sourcepaths++;
        while(1){
            paths[(*pathlen)++] = sourcepaths;
            while(*sourcepaths != ':' && *sourcepaths != '\0'){
                sourcepaths++;
            }
            if(*sourcepaths == '\0'){
                return;
            }
            *sourcepaths = '\0';
            sourcepaths++;
        }
}

void createhistorylist(struct history_list *hlist, int *histfd){
    struct history_list *buflist;
    if((*histfd = open(".mysh_history", (O_RDWR | O_CREAT | O_EXCL),0755)) < 0){
        if(errno != EEXIST){
            perror("open");
            exit(1);
        }
        if((*histfd = open(".mysh_history", (O_RDWR | O_CREAT |O_APPEND),0755)) < 0){
            perror("open");
            exit(1);
        }else{
            int rd;
            char hbuf[1];
            int bufindex = 0;
            char tempbuf[BUFLEN];
            memset(tempbuf, '\0', BUFLEN);

            while((rd = read(*histfd, hbuf, 1))){
              if(rd < 0){
                perror("read");
                close(*histfd);
                exit(1);
              }
              if(hbuf[0] != '\n'){
                  tempbuf[bufindex++] = hbuf[0];
              }else{
                  tempbuf[bufindex] = '\0';
                  buflist = (struct history_list*)malloc(sizeof(struct history_list));
                  if(buflist == NULL){
                      fprintf(stderr, "Could not allocate space for history\n");
                      exit(1);
                  }
                  strcpy(buflist->command, tempbuf);
                  (hlist->bp)->fp = buflist;
                  buflist->bp = hlist->bp;
                  hlist->bp = buflist;
                  buflist->fp = hlist;
                  (hlist->len)++;
                  buflist->len = hlist->len;
                  memset(tempbuf, '\0', BUFLEN);
                  bufindex = 0;
              }
          }
        }
    }
}
