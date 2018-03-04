//61510180 鈴木慶汰
#include "shell.h"

//Prototypes
void enableRawMode();
void disableRawMode();
int readline(char *buf, struct history_list *hlist, char *env[], int envlen);
int determinearrow(struct bufchars *bufheadc, int *cursorpos, struct history_list **curhislist, struct history_list *hlist, struct bufchars **cursorbuf);
void swaptohistory(struct bufchars *bufheadc, int *cursorpos, struct history_list **curhislist, struct history_list *hlist, int ab);
void getdirectoryinfo(struct directory_info *dlist, struct bufchars *bufheadc, struct bufchars *curbufchar, int *cursorpos, char *env[], int envlen);

//Global Vars
struct termios orig_termios;
int HISTCOUNTER;

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) < 0){
        perror("tcsetattr");
        exit(1);
    }
}
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
      perror("tcgetattr");
      exit(1);
    }
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO| ICANON | IEXTEN | ISIG);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
      perror("tcsetattr");
      exit(1);
    }
}

int readline(char *buf, struct history_list *hlist, char *env[], int envlen){
    int i;
    int cursorpos = 0;
    int refresh = 1;
    int tabcount = 2;
    char c;
    struct bufchars bufheadc, *stringbuf, *ibuf, *cursorbuf;
    bufheadc.fp = bufheadc.bp = &bufheadc;
    bufheadc.len = 0;
    struct history_list *curhislist;
    struct directory_info dlist, *idlist;
    curhislist = hlist;
    cursorbuf = &bufheadc;
    HISTCOUNTER = 0;

    for(i = 0; i < BUFLEN; i++){
        buf[i] = '\0';
    }

    printf("mysh$");
    while(1){
        refresh = 1;
        c = getchar();
        switch(c){
            case CTRL_KEY('M'):
                //Enter key
                tabcount = 2;
                for(ibuf = bufheadc.fp, i = 0; ibuf != &bufheadc && i < BUFLEN; ibuf = ibuf->fp, i++){
                    buf[i] = ibuf->c;
                }
                if(i < BUFLEN){
                    buf[i] = '\n';
                }else{
                    buf[BUFLEN - 1] = '\n';
                }
                for(ibuf = bufheadc.fp; bufheadc.fp != &bufheadc; ibuf = bufheadc.fp){
                    bufheadc.fp = ibuf->fp;
                    (ibuf->fp)->bp = &bufheadc;
                    (bufheadc.len)--;
                    if(cursorpos != 0){
                        cursorpos--;
                    }
                    free(ibuf);
                }
                return 1;
                break;
            case 9:
            case 11:
                //tabs
                tabcount--;
                getdirectoryinfo(&dlist, &bufheadc, cursorbuf, &cursorpos, env, envlen);
                if(tabcount == 0 && dlist.ttl > 1){
                    if(dlist.ttl > 10){
                        int smallflag = 0;
                        printf("\n\rDisplay all %d possibilities?(y/n):", dlist.ttl);
                        while(1){
                            c = getchar();
                            if(c == 'y'){
                                printf("y");
                                break;
                            }else if(c == 'n'){
                                printf("n\n");
                                smallflag = 1;
                                break;
                            }else{
                                printf("\a");
                            }
                        }
                        if(smallflag){
                            for(idlist = dlist.fp; dlist.fp != &dlist; idlist = dlist.fp){
                                dlist.fp = idlist->fp;
                                (idlist->fp)->bp = &dlist;
                                free(idlist);
                            }
                            tabcount = 2;
                            break;
                        }
                    }
                    if(dlist.fp != &dlist && dlist.bp != &dlist){
                        printf("\n");
                        printf("\r");
                        for(idlist = dlist.fp; dlist.fp != &dlist; idlist = dlist.fp){
                            printf("%s ", idlist->name);
                            dlist.fp = idlist->fp;
                            (idlist->fp)->bp = &dlist;
                            free(idlist);
                        }
                        printf("\n");
                    }
                    tabcount = 2;
                }
                break;
            case '\x1b':
                //Escape Sequence ex)Arrows
                tabcount = 2;
                c = getchar();
                if(c == '['){
                    refresh = determinearrow(&bufheadc, &cursorpos, &curhislist, hlist, &cursorbuf);
                }
                break;
            case CTRL_KEY('p'):
                //Get Prev history
                tabcount = 2;
                swaptohistory(&bufheadc, &cursorpos, &curhislist, hlist, 0);
                break;
            case CTRL_KEY('q'):
            case CTRL_KEY('c'):
                //Quit
                tabcount = 2;
                exit(1);
            case 127:
            case CTRL_KEY('H'):
                //Delete
                tabcount = 2;
                if(bufheadc.len > 0 && cursorpos > 0){
                    bufheadc.len--;
                    cursorpos--;
                    (cursorbuf->bp)->bp->fp = cursorbuf;
                    cursorbuf->bp = (cursorbuf->bp)->bp;
                }else{
                    printf("\a");
                }
                break;
            default:
                tabcount = 2;
                if(iscntrl(c)){
                    break;
                }
                stringbuf = (struct bufchars*)malloc(sizeof(struct bufchars));
                if(stringbuf == NULL){
                    fprintf(stderr, "Could not allocate new character.\n");
                    exit(1);
                }
                stringbuf->c = c;
                (cursorbuf->bp)->fp = stringbuf;
                stringbuf->bp = cursorbuf->bp;
                cursorbuf->bp = stringbuf;
                stringbuf->fp = cursorbuf;
                (bufheadc.len)++;
                cursorpos++;
                break;
        }
        if(refresh){
            printf("\x1b[?25l");
            printf("\x1b[2K");
            printf("\rmysh$");
            for(ibuf = bufheadc.fp; ibuf != &bufheadc; ibuf = ibuf->fp){
                printf("%c", ibuf->c);
            }
            printf("\x1b[?25h");
            if(bufheadc.len != cursorpos && cursorpos >= 0){
               printf("\x1b[%dD", bufheadc.len - cursorpos);
            }
        }
    }
}

int determinearrow(struct bufchars *bufheadc, int *cursorpos, struct history_list **curhislist, struct history_list *hlist, struct bufchars **cursorbuf){
    char c;
    struct bufchars *ibuf;

    c = getchar();
    switch(c){
        case ARROWUP:
            swaptohistory(bufheadc, cursorpos, curhislist, hlist, 0);
            return 1;
            break;
        case ARROWDOWN:
            swaptohistory(bufheadc, cursorpos, curhislist, hlist, 1);
            return 1;
            break;
        case ARROWLEFT:
            if(*cursorpos > 0){
                (*cursorpos)--;
                (*cursorbuf) = (*cursorbuf)->bp;
                printf("\x1b[1D");
            }else{
                printf("\a");
            }
            return 0;
            break;
        case ARROWRIGHT:
            if(*cursorpos < bufheadc->len){
                (*cursorpos)++;
                (*cursorbuf) = (*cursorbuf)->fp;
                printf("\x1b[1C");
            }else{
                printf("\a");
            }
            return 0;
            break;
        default:
            return 1;
            break;
    }
}

void swaptohistory(struct bufchars *bufheadc, int *cursorpos, struct history_list **curhislist, struct history_list *hlist, int ab){
    struct bufchars *ibuf, *stringbuf;
    int i;
    if(HISTCOUNTER == 0 && ab){
        printf("\a");
        return;
    }
    if(HISTCOUNTER == hlist->len && !ab){
        printf("\a");
        return;
    }
    for(ibuf = bufheadc->fp, i =0; bufheadc->fp != bufheadc && i < BUFLEN; ibuf = bufheadc->fp, i++){
        (*curhislist)->command[i] = ibuf->c;
        bufheadc->fp = ibuf->fp;
        (ibuf->fp)->bp = bufheadc;
        (bufheadc->len)--;
        if(cursorpos != 0){
            (*cursorpos)--;
        }
        free(ibuf);
    }
    (*curhislist)->command[(i < BUFLEN ? i:(BUFLEN-1))] = '\0';
    if(ab){
            *curhislist = (*curhislist)->fp;
            HISTCOUNTER--;
    }else{
            (*curhislist) = (*curhislist)->bp;
            HISTCOUNTER++;
    }

    for(i = 0; i < strlen((*curhislist)->command); i++){
        stringbuf = (struct bufchars*)malloc(sizeof(struct bufchars));
        if(stringbuf == NULL){
            fprintf(stderr, "Could not allocate new character.\n");
            exit(1);
        }
        stringbuf->c = (*curhislist)->command[i];
        (bufheadc->bp)->fp = stringbuf;
        stringbuf->bp = bufheadc->bp;
        bufheadc->bp = stringbuf;
        stringbuf->fp = bufheadc;
        (bufheadc->len)++;
        (*cursorpos)++;
    }
}

void getdirectoryinfo(struct directory_info *dlist, struct bufchars *bufheadc, struct bufchars *curbufchar, int *cursorpos, char *env[], int envlen){
    DIR *dir;
    struct dirent *dp;
    char path[BUFLEN * 2], temppath[BUFLEN], match_dir[BUFLEN], compedname[BUFLEN];
    struct directory_info *bufdir, *ibufdir;
    struct bufchars *ibuf, *startpos, *stringbuf;
    int space, pipe, dirty, mode, slash, currentspace;
    int i;
    space = 0;
    currentspace = 0;
    dirty = 0;
    pipe = 0;
    mode = 0;
    slash = 0;
    startpos = bufheadc->fp;

    dlist->fp = dlist->bp = dlist;
    dlist->ttl = 0;
    memset(path, '\0', BUFLEN * 2);
    memset(temppath, '\0', BUFLEN);
    memset(match_dir, '\0', BUFLEN);
    memset(compedname, '\0', BUFLEN);

    for(ibuf = ((curbufchar != bufheadc) ? curbufchar:curbufchar->bp); ibuf != bufheadc; ibuf = ibuf->bp){
        if(ibuf->c == ' '){
            dirty++;
            if(startpos == bufheadc->fp){
                currentspace = space;
                startpos = ibuf->fp;
            }
        }else if(ibuf->c == '|'){
            pipe++;
            if(startpos == bufheadc->fp){
                startpos = ibuf->fp;
            }
            break;
        }else{
            if(ibuf->c == '/' && startpos == bufheadc->fp){
                slash++;
            }
            if(dirty > 0){
                space++;
            }
            dirty = 0;
        }
    }
    mode = (currentspace == space) ? MODEPATH : MODEDIR;

    for(ibuf = startpos, i = 0; i < BUFLEN && ibuf != curbufchar; ibuf = ibuf->fp, i++){
        if(slash > 0){
            temppath[i] = ibuf->c;
        }else if(slash == 0){
            i = 0;
            match_dir[i] = ibuf->c;
            slash--;
        }else{
            match_dir[i] = ibuf->c;
        }

        if(ibuf->c == '/'){
            slash--;
        }
        if((ibuf->fp)->c == ' '|| (ibuf->fp)->c == '|'){
            break;
        }
    }
    if(mode == MODEPATH){
        for(i = 0; i < envlen; i++){
            dir = opendir(env[i]);
            if(dir == NULL){
                //fprintf(stderr, "Could not open path %s\n", env[i]);
                continue;
            }
            dp = readdir(dir);
            while (dp != NULL) {
                if(strncmp(dp->d_name,match_dir, strlen(match_dir)) == 0){
                    bufdir = (struct directory_info*)malloc(sizeof(struct directory_info));
                    strncpy(bufdir->name, dp->d_name, BUFLEN);
                    (dlist->bp)->fp = bufdir;
                    bufdir->bp = dlist->bp;
                    dlist->bp = bufdir;
                    bufdir->fp = dlist;
                    (dlist->ttl)++;
                }
                dp = readdir(dir);
            }
            closedir(dir);
        }
        //search env all paths
    }else{
        if(startpos == bufheadc || startpos->c == ' '){
            getcwd(path, BUFLEN * 2);
        }else{
            switch(temppath[0]){
                case '/':
                case '.':
                    strncpy(path, temppath, BUFLEN * 2);
                    break;
                case '~':
                    strncpy(path, getenv("HOME"), BUFLEN);
                    if(temppath[1] == '/'){
                        strncat(path, temppath + 1, BUFLEN * 2 - strlen(path));
                    }else{
                        strcat(path, "/");
                        strncat(path, temppath + 2, BUFLEN * 2 - strlen(path));
                    }
                    break;
                default:
                    getcwd(path, BUFLEN * 2);
                    strcat(path, "/");
                    strncat(path, temppath, BUFLEN * 2);
                    break;
            }
        }
    dir = opendir(path);
    if(dir == NULL){
        fprintf(stderr, "Could not Open current path");
        return;
    }
    dp = readdir(dir);
    while (dp != NULL) {
        if(strncmp(dp->d_name,match_dir, strlen(match_dir)) == 0){
            bufdir = (struct directory_info*)malloc(sizeof(struct directory_info));
            strncpy(bufdir->name, dp->d_name, BUFLEN);
            if(dp->d_type == DT_DIR){
                strncat(bufdir->name, "/", BUFLEN);
            }
            (dlist->bp)->fp = bufdir;
            bufdir->bp = dlist->bp;
            dlist->bp = bufdir;
            bufdir->fp = dlist;
            (dlist->ttl)++;
        }
        dp = readdir(dir);
    }
    closedir(dir);
    }

    if(dlist->ttl == 1){
        for(i = strlen(match_dir); i < strlen((dlist->fp)->name) ; i++){
            stringbuf = (struct bufchars*)malloc(sizeof(struct bufchars));
            if(stringbuf == NULL){
                fprintf(stderr, "Could not allocate new character.\n");
                exit(1);
            }
            stringbuf->c = ((dlist->fp)->name)[i];
            (curbufchar->bp)->fp = stringbuf;
            stringbuf->bp = curbufchar->bp;
            curbufchar->bp = stringbuf;
            stringbuf->fp = curbufchar;
            (bufheadc->len)++;
            (*cursorpos)++;
        }
        free(dlist->fp);
        dlist->fp = dlist->bp = dlist;
    }else if(dlist->ttl > 0 && strlen(match_dir) > 0){
        strcpy(compedname,(dlist->fp)->name);
        for(ibufdir = dlist->fp; ibufdir != dlist; ibufdir = ibufdir->fp){
            int flag = 0;
            for(i = strlen(match_dir) - 1; i < strlen(ibufdir->name); i++){
                if(strncmp(ibufdir->name, compedname, i) != 0){
                    flag = 1;
                    break;
                }
            }
            i = flag ? i : (i + 1);
            strncpy(compedname, ibufdir->name, i - 1);
            compedname[i - 1] = '\0';
        }
        for(i = strlen(match_dir); i < strlen(compedname) ; i++){
            stringbuf = (struct bufchars*)malloc(sizeof(struct bufchars));
            if(stringbuf == NULL){
                fprintf(stderr, "Could not allocate new character.\n");
                exit(1);
            }
            stringbuf->c = compedname[i];
            (curbufchar->bp)->fp = stringbuf;
            stringbuf->bp = curbufchar->bp;
            curbufchar->bp = stringbuf;
            stringbuf->fp = curbufchar;
            (bufheadc->len)++;
            (*cursorpos)++;
        }
    }
}
