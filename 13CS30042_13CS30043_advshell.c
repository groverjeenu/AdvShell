// Assignment 2
// Q2. Advanced Shell

// Objective
// The shell is a program that interprets commands and acts as an intermediary between the user and the inner workings of the
// operating system and as such is arguably one of the most important parts of a Unix system.
// In this assignment, we shall start making our very own version of a Unix shell.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <langinfo.h>
#include <errno.h>

int handle_reverse = 0;

char* escape(char* buffer)
{
    int i,j;
    int l = strlen(buffer) + 1;
    char* dest  =  (char*)malloc(1024*sizeof(char));
    char* ptr=dest;
    for(i=0; i<l; i++)
    {
        if( buffer[i]=='\\' )
        {
            continue;
        }
        *ptr++ = buffer[i];
    }
    *ptr = '\0';
    return dest;
}

char *preprocess(char *buffer)
{
    int i,j;
    int l = strlen(buffer) + 1;
    char* dest  =  (char*)malloc(1024*sizeof(char));
    char* ptr=dest;
    i = 0;
    *ptr++ = buffer[i];
    for(i=1; i<l; i++)
    {
        if( (buffer[i]=='<' || buffer[i] == '>' || buffer[i] == '|' ) && (buffer[i-1] != ' '))
        {
            *ptr++ = ' ';
        }

        if((buffer[i-1]=='<' || buffer[i-1] == '>' || buffer[i-1] == '|' ) && (buffer[i] != ' '))
        {
            *ptr++ = ' ';
        }
        *ptr++ = buffer[i];
    }
    *ptr = '\0';
    return dest;
}

// Function to parse the entered command
char ** lexer(char * buffer,int * cnt,int *left,int *right,int *vertical,int *l_count,int *r_count,int *v_count)
{
    int i = 1,count  = 0,start = 0,f = 0;

    *l_count = 0;
    *r_count = 0;
    *v_count = 0;

    char ** a = (char**)malloc(1024*sizeof(char*));
    if(buffer[0] != ' ')
        start = 0;
    while(1)
    {
        if(buffer[i] == '>')
        {
            right[*r_count] = count;
            (*r_count)++;
        }
        if(buffer[i] == '<')
        {

            left[*l_count] = count;
            (*l_count)++;
        }
        if(buffer[i] == '|')
        {
            vertical[*v_count] = count;
            (*v_count)++;
        }

        if(buffer[i-1] == '\\' && buffer[i] == ' ')
        {
            f = 1;
            i = i+2;
            continue;
        }
        if((buffer[i] == ' ' || buffer[i] == '\n') && buffer[i-1] != ' ')
        {
            a[count++] = (char*)malloc(256*sizeof(char));
            strncpy(a[count-1],buffer + start,i - start);
            a[count-1][i-start] = '\0';
            if(f == 1)
            {
                strcpy(a[count-1],escape(a[count-1]));
                f = 0;
            }
        }
        else if(buffer[i] != ' ' && buffer[i-1] == ' ')
        {
            start = i;

        }
        if(buffer[i] == '\n')
            break;
        i++;
    }
    a[count] = NULL;
    *cnt  = count;
    return a;
}

// Function for executing clear command
void clear()
{
    printf("\033[H\033[J");
}

void printTime()
{
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
    puts(buffer);
    return;
}

// Function for executing cd(change directory) command
void changeDir(const char *path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        printf("%s: No such file or directory\n",path);
    }
    else
    {
        chdir(path);
    }
}

// Function for executing pwd command
void pwd()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);

}

// Function for executing mkdir(make directory) command
void makeDir(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        mkdir(path, 0700);
    }
    else
    {
        printf("cannot create directory %s: File exists\n",path);
    }

}

// Function for executing ls command
void ls(const char *currDir)
{
    DIR *directory;
    struct dirent *dirPtr;
    int i;

    if(currDir == NULL)
    {
        printf("Error: Could not get the working directory\n");
        return;
    }

    directory = opendir((const char*)currDir);
    if(directory == NULL)
    {
        printf("Error: Could not open the working directory\n");
    }

    for(i = 0; (dirPtr = readdir(directory))!=NULL; i++)
    {
        printf("%s\n",dirPtr->d_name);
    }
    printf("\n");
}

// Function for executing ls -l command
void lsl(const char *currDir)
{
    DIR *directory;
    struct dirent *dirPtr;
    struct stat fileStat;
    char *path;
    struct passwd *pwd;
    struct group *grp;
    struct tm *tm;
    char datestring[1024];
    int i;

    path = (char *)malloc(1024*sizeof(char));

    if(currDir == NULL)
    {
        printf("Error: Could not get the working directory\n");
        return;
    }

    directory = opendir((const char*)currDir);
    if(directory == NULL)
    {
        printf("Error: Could not open the working directory\n");
    }

    for(i = 0; (dirPtr = readdir(directory))!=NULL; i++)
    {
        sprintf(path, "%s/%s", currDir, dirPtr->d_name);
        stat(path, &fileStat);  // Get the fileStat

        // Print the Permissions
        printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
        printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");

        printf("\t%zu\t",fileStat.st_nlink);    // Print Number of links

        // Print user name
        if ((pwd = getpwuid(fileStat.st_uid)) != NULL) printf("%s\t", pwd->pw_name);
        else printf("%d\t", fileStat.st_uid);

        // Print out group name
        if ((grp = getgrgid(fileStat.st_gid)) != NULL)
            printf("%s\t", grp->gr_name);
        else
            printf("%d\t", fileStat.st_gid);

        printf("%jd\t",fileStat.st_size);   // Print size of the file
        tm = localtime(&fileStat.st_mtime); // Print Last modified Datetime

        strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);

        printf("%s\t", datestring);
        printf("%s\n",dirPtr->d_name);  // Print Filename
    }
    printf("\n");
}


// Function for executing rmdir(remove directory) command
void removeDir(const char *path)
{
    if(rmdir(path))
    {
        printf("Directory could not be deleted\n");
    }
}

// Function for executing history command
void history()
{
    char temp[1024];
    FILE *fp;
    char *buffer;
    size_t size = 1024;

    strcpy(temp,getenv("HOME"));
    strcat(temp,"/history.txt");

    fp = fopen(temp,"r");

    buffer = (char *)malloc(1024*sizeof(char));

    if (fp == NULL)
        printf("File not found\n");
    int ctr = 1;

    while (getline(&buffer, &size, fp) != -1)
    {
        printf("\t%d\t%s",ctr++, buffer);
    }
    fclose(fp);
}

// Function for executing history <Argument> command
void historyArg(int n)
{
    FILE *fp;
    char **buffer;
    int i;
    size_t size = 1024;
    buffer = (char **)malloc(10000*sizeof(char *));
    for(i=0; i<10000; ++i)
    {
        buffer[i] = (char *)malloc(size*sizeof(char));
    }
    char temp[400];
    strcpy(temp,getenv("HOME"));
    strcat(temp,"/history.txt");
    fp = fopen(temp, "r");
    if (fp == NULL)
        printf("File not found\n");
    int ctr = 1;

    while (getline(&(buffer[ctr]), &size, fp) != -1)
    {
        ctr++;
    }
    if(ctr-n<=0)
    {
        i = 1;
    }
    else i = ctr-n;
    for(; i<ctr; ++i)
    {
        printf("\t%d\t%s",i, buffer[i]);
    }

    fclose(fp);
}


// Reverse-Search Handler Function
void reverse_search()
{
    handle_reverse = 1;
    char temp[1024];
    FILE *fp;
    char **buffer,*cmd,*currDir;
    size_t size = 1024;
    size_t prefLen,strLen;
    int i;
    currDir = (char *)malloc(1024*sizeof(char));
    strcpy(temp,getenv("HOME"));
    strcat(temp,"/history.txt");

    fp = fopen(temp,"r");
    buffer = (char **)malloc(10000*sizeof(char *));
    for(i=0; i<10000; ++i)
    {
        buffer[i] = (char *)malloc(size*sizeof(char));
    }
    cmd = (char *)malloc(1024*sizeof(char));

    if (fp == NULL)
        printf("File not found\n");

    printf("\nReverse-Search:");

    int ctr = 0,ii = 0;

    while((cmd[ii] = getchar()) != '\n' && cmd[ii] != EOF) ii++;

    while (getline(&(buffer[ctr]), &size, fp) != -1)
    {
        ctr++;
    }

    cmd[strlen(cmd)-1] = '\0';
    prefLen = strlen(cmd);


    for(i=ctr-1; i>=0; --i)
    {
        strLen = strlen(buffer[i]);
        if(strLen<prefLen) continue;
        if(strncmp(cmd,buffer[i],prefLen) == 0) printf("%s",buffer[i]);
    }
    fclose(fp);
}

// Redirect using '<'
void redirectL(char *cmd,char **args,char *filename)
{
    int pid = fork(),status;
    if(pid == 0)
    {
        int in = open(filename,O_RDONLY);
        close(STDIN_FILENO);
        dup2(in,STDIN_FILENO);
        close(in);
        if(execvp(cmd, args) == -1)printf("%s : Command Not Found\n",cmd);
    }
    else
    {
        waitpid(pid,&status,0);
    }

}

// Redirect using '>'
void redirectR(char *cmd,char **args,char *filename)
{
    int pid = fork(),status;
    if(pid == 0)
    {
        int out = open(filename,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        close(STDOUT_FILENO);
        dup2(out,STDOUT_FILENO);
        close(out);
        if(execvp(cmd, args) == -1) printf("%s : Command Not Found\n",cmd);
    }
    else
    {
        waitpid(pid,&status,0);
    }
}

// Use both '<' and '>' for redirecting
void redirectLR(char *cmd,char **args,char *inFN,char *outFN)
{
    int pid = fork(),status;
    if(pid == 0)
    {
        int in = open(inFN,O_RDONLY);
        int out = open(outFN,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(out,STDOUT_FILENO);
        dup2(in,STDIN_FILENO);
        close(out);
        close(in);
        if(execvp(cmd, args) == -1) printf("%s : Command Not Found\n",cmd);
    }
    else
    {
        waitpid(pid,&status,0);
    }
}


// Redirect using '|'
void redirectV2(char ***cmd)
{
    int p[2];
    pipe(p);

    //printf("00: %s\n",cmd[0][0]);
    //printf("01: %s\n",cmd[0][1]);
    //printf("10: %s\n",cmd[1][0]);
    //printf("11: %s\n",cmd[1][1]);

    if(fork() == 0)
    {
        close(STDOUT_FILENO);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        execvp(cmd[0][0], cmd[0]);
        perror("execvp of ls failed");
        exit(1);
    }

    if(fork() == 0)
    {
        close(STDIN_FILENO);
        dup(p[0]);
        close(p[1]);
        close(p[0]);

        execvp(cmd[1][0], cmd[1]);
        perror("execvp of wc failed");
        exit(1);
    }

    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
}

// Redirect using '|'
void redirectVn(char ***cmd,int n)
{
    int *p;
    p = (int *)malloc((2*(n-1))*sizeof(int));
    int i,cnt = 0;
    for(i=0; i<n-1 ; ++i)
    {
        pipe(p+cnt);
        cnt = cnt+2;
    }
    //printf("n: %d\n",n);
    //printf("00: %s\n",cmd[0][0]);
    //printf("01: %s\n",cmd[0][1]);
    //printf("10: %s\n",cmd[1][0]);
    //printf("11: %s\n",cmd[1][1]);

    int cmd_no = 0,temp = n;

    while(temp--)
    {
        //printf("cmd: %s\n",cmd[cmd_no][0]);
        if(fork() == 0)
        {
            if(cmd_no!=0)
            {
                close(STDIN_FILENO);
                dup(p[(cmd_no-1)*2]);
            }

            if(temp!=0)
            {
                close(STDOUT_FILENO);
                dup(p[(cmd_no*2)+1]);
            }
            for(i=0; i<(2*(n-1)); ++i)
            {
                close(p[i]);
            }

            execvp(cmd[cmd_no][0], cmd[cmd_no]);
            printf("Executing: %s",cmd[cmd_no][0]);
            perror("execvp failed");
            exit(1);
        }


        cmd_no++;
    }

    for(i=0; i<(2*(n-1)); ++i)
    {
        close(p[i]);
    }

    for(i=0; i<(2*(n-1)); ++i)
    {
        wait(0);
    }
}

int main(int argc, char *argv[], char *envp[])
{
    int cnt,i,j,k,envIdx;
    char homeDir[1024],temp[1024],currDir[1024];

    char ***pipe_cmd;

    int left[1024],right[1024],vertical[1024];
    int v_count,l_count,r_count;

    FILE *fp = NULL;

    strcpy(homeDir,getenv("HOME"));
    strcpy(temp,homeDir);
    strcat(temp,"/history.txt");    // Path for history file

    size_t size = 1024;
    char *buffer_old = (char * )malloc(size*sizeof(char));
    char *buffer = (char * )malloc(size*sizeof(char));
    char **cmd;
    struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &reverse_search;

    changeDir(homeDir); // Set current working directory as home directory



    while(1)
    {
        char *buffer_old = (char * )malloc(size*sizeof(char));
        char *buffer = (char * )malloc(size*sizeof(char));

        sigaction(SIGQUIT, &act1, NULL);

        if(handle_reverse == 1)
        {
            handle_reverse = 0;
            continue;
        }

        getcwd(currDir, sizeof(currDir));   // Get Current Working Directory
        printf("%s >",currDir);
        if(handle_reverse == 1)
        {
            handle_reverse = 0;
            continue;
        }
        int ii = 0;

        while((buffer_old[ii] = getchar()) != '\n' && buffer_old[ii] != EOF) ii++; // Get Input from the user

        if(handle_reverse == 1)
        {
            handle_reverse = 0;
            continue;
        }

        fp =fopen(temp,"a");            // Open the history file

        if(strcmp(buffer_old,"\n") == 0)
        {
            continue;                   // Ignore Execution of '\n' command
        }

        strcpy(buffer,preprocess(buffer_old));

        //printf("%s\n",buffer);

        cmd = lexer(buffer,&cnt,left,right,vertical,&l_count,&r_count,&v_count);       // Parse the command entered



        if(cnt > 0) fprintf(fp,"%s",buffer);    // Print the command in history file
        fclose(fp);                     // Close the history file



        // Check for redirection
        if(l_count>0)
        {
            if(r_count>0)
            {
                //printf("LR: %s\t%s\n",cmd[left[0]+1],cmd[right[0]+1]);
                redirectLR(cmd[0],&cmd[0],cmd[left[0]+1],cmd[right[0]+1]);
            }
            else
            {
                //printf("L: %s\n",cmd[left[0]+1]);
                redirectL(cmd[0],&cmd[0],cmd[left[0]+1]);
            }
        }

        else if(r_count>0)
        {
            //printf("R: %s\n",cmd[right[0]+1]);
            redirectR(cmd[0],&cmd[0],cmd[right[0]+1]);
        }

        else if(v_count>0)
        {
            pipe_cmd = (char ***)malloc((v_count+1)*sizeof(char **));
            for(i=0; i<=v_count; ++i)
            {
                pipe_cmd[i] = (char **)malloc((cnt+1)*sizeof(char *));
            }

            //printf("R: %s\n",cmd[right[0]+1]);
            int last = 0;
            int pipe_cnt = 0;
            int pipe_cnt_out = 0;
            int k;
            for(j=0; j<v_count; ++j)
            {
                for(k=last; k<vertical[j]; ++k)
                {
                    pipe_cmd[pipe_cnt_out][pipe_cnt] = NULL;
                    //pipe_cmd[pipe_cnt_out][pipe_cnt] = (char *)malloc(size*sizeof(char));
                    //printf("cmd: %s\n",cmd[k]);
                    //strcpy(pipe_cmd[pipe_cnt_out][pipe_cnt],cmd[k]);

                    pipe_cmd[pipe_cnt_out][pipe_cnt] = cmd[k];
                    pipe_cmd[pipe_cnt_out][pipe_cnt][strlen(cmd[k])] = '\0';
                    pipe_cnt++;
                }
                pipe_cmd[pipe_cnt_out][pipe_cnt] = NULL;
                pipe_cnt_out++;
                last = vertical[j]+1;
                pipe_cnt = 0;
            }
            for(k=last; k<cnt; ++k)
            {
                pipe_cmd[pipe_cnt_out][pipe_cnt] = NULL;
                //pipe_cmd[pipe_cnt_out][pipe_cnt] = (char *)malloc(size*sizeof(char));
                //strcpy(pipe_cmd[pipe_cnt_out][pipe_cnt],cmd[k]);
                pipe_cmd[pipe_cnt_out][pipe_cnt] = cmd[k];
                pipe_cmd[pipe_cnt_out][pipe_cnt][strlen(cmd[k])] = '\0';
                pipe_cnt++;
            }
            pipe_cmd[pipe_cnt_out][pipe_cnt] = NULL;

            //printf("00: %s\n",pipe_cmd[0][0]);
            //printf("01: %s\n",pipe_cmd[0][1]);
            //printf("10: %s\n",pipe_cmd[1][0]);
            //printf("11: %s\n",pipe_cmd[1][1]);
            //redirectV2(pipe_cmd);
            if(pipe_cnt_out>=1)
                redirectVn(pipe_cmd,(pipe_cnt_out+1));

            else redirectV2(pipe_cmd);
            free(pipe_cmd);
        }


        // Handling 'clear' command
        else if(strcmp(cmd[0],"clear") == 0)
        {
            clear();
        }

        // Handling 'env' command
        else if(strcmp(cmd[0],"env") == 0)
        {
            if(cnt == 1)
            {
                envIdx = 0;
                while (envp[envIdx])
                {
                    printf("%s\n", envp[envIdx++]);
                }
            }
            else
            {
                printf("%s: Command not found\n",buffer);
            }
        }

        // Handling 'cd' command
        else if(strcmp(cmd[0],"cd") == 0)
        {
            if(cnt == 1)
            {
                changeDir(homeDir);
            }
            else
            {
                changeDir(cmd[1]);
            }
        }

        // Handling 'pwd' command
        else if(strcmp(cmd[0],"pwd") == 0)
        {
            pwd();
        }

        // Handling 'mkdir' command
        else if(strcmp(cmd[0],"mkdir") == 0)
        {
            if(cnt == 1)
            {
                printf("mkdir: missing operand\n");
            }
            else
            {
                for(j=1; j<cnt; j++)
                    makeDir(cmd[j]);
            }
        }

        // Handling 'ls' and 'ls -l' command
        else if(strcmp(cmd[0],"ls") == 0)
        {
            if(cnt == 1)
            {
                ls(currDir);
            }
            else
            {
                if(strcmp(cmd[1],"-l")==0)
                {
                    lsl(currDir);
                }
            }
        }

        // Handling 'rmdir' command
        else if(strcmp(cmd[0],"rmdir") == 0)
        {
            if(cnt == 1)
            {
                printf("rmdir: missing operand\n");
            }
            else
            {
                for(j=1; j<cnt; j++)
                    removeDir(cmd[j]);
            }
        }

        // Handling 'history' command
        else if(strcmp(cmd[0],"history") == 0)
        {
            if(cnt == 1)
            {
                history();
            }
            else if(cnt == 2)
            {
                historyArg(atoi(cmd[1]));
            }
            else
            {
                printf("history: Too Many Arguments\n");
            }
        }

        // Handling 'exit' command
        else if(cnt ==1 && strcmp(cmd[0],"exit") == 0) return 0;

        // Handling background execution of commands
        else if(cmd[0][strlen(cmd[0])-1] == '&')
        {
            int pid = fork(),status;
            cmd[0][strlen(cmd[0])-1] = '\0';
            if(pid == 0)
            {
                if(execvp(cmd[0], &cmd[0]) == -1)printf("%s : Command Not Found\n",cmd[0]);
            }
        }

        // Handling background execution of commands
        else if(strcmp(cmd[cnt-1],"&") == 0)
        {
            int pid = fork(),status;
            cmd[0][strlen(cmd[0])] = '\0';
            if(pid == 0)
            {
                if(execvp(cmd[0], &cmd[0]) == -1)printf("%s : Command Not Found\n",cmd[0]);
            }
        }

        //Handling normal execution of commands
        else
        {
            int pid = fork(),status;
            if(pid == 0)
            {
                if(execvp(cmd[0], &cmd[0]) == -1)printf("%s : Command Not Found\n",cmd[0]);
            }
            else
            {
                waitpid(pid,&status,0);
            }
        }

    }

}


