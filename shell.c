#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<string.h>
#include <errno.h> // strerror 
#include <signal.h>


#define max_size 4096 // 2^12

char current_directory[1000];
char input_command[max_size];
int len_command;
int exit_flag;
int wait_flag = 0;
int background_flag = 0;
char file_name[] = "log.txt";

void prompt();
void reset_all();
void process_command(char *current_command, int len_command);
void *remove_spaces_and_next_line_at_end(char *command);
void launch_progam(char **args, int background);
void intro();
void handler(int signal);
void log_file(int die, pid_t id, int fg);

int main()
{
    intro();

    // create a txt file if not created
    FILE *fp = fopen(file_name, "ab+");
    // clear the content of the txt file
    fclose(fopen(file_name, "w"));

    int status;
    char ch[2]={"\n"};


    reset_all();
    signal(SIGCHLD, handler);
    while(1)
    {
        prompt();

        // we used fgets instead of fgets and scanf 
        // because it checks the boundary of the input string so it's safer to use
        // it reads the '\n' as part of the string
        // scanf doesn't take spaces but don't take the '\n' in the string
        fgets(input_command, max_size ,stdin);
        len_command = strlen(input_command);

        remove_spaces_and_next_line_at_end(input_command);

        // strcmp return 0 in case the two compared string is the same
        if(!strcmp(input_command, ""))
        {
            printf("command empty\n");
        }
        else
        {
            process_command(input_command, len_command);
            // background_flag = 0;
            if(exit_flag==1)
            {
                printf("shell finished and terminated\n");
                exit(0);
                return 0;
            }
        }

        if(wait_flag==1)
        {
            return 0;
        }
    }
    if(exit_flag==1)
    {
        printf("shell finished and terminated\n");
        exit(0);
        return 0;
    }
    return 0;
}

void log_file(int die, pid_t id, int fg)
{
    FILE *fp = fopen(file_name, "a");
    if (fp == NULL)
    {
        printf("Error opening the log file %s", file_name);
        // return -1;
    }
    // write to the text file
    if(die==0)
    {
        if(fg==0)
        {
            fprintf(fp, "Child pid %d started in the background\n", id);
        }
        else if(fg==1)
        {
            fprintf(fp, "Child pid %d started in the foreground\n", id);
        }
    }
    else if(die==1)
    {
        fprintf(fp, "Child process was terminated\n");
    }
    // close the file
    fclose(fp);
}

void handler(int signal)
{
    pid_t chpid = wait(NULL);

    log_file(1,chpid,0);
    // log_file(1,getpid(),0); // parent id
}

void reset_all()
{
    exit_flag = 0;
    len_command = 0;
    wait_flag = 0;
    background_flag = 0;
    // printf("all variable are reseted\n");
}

void process_command(char *current_command, int len_command)
{
    int i=0, j=0;

    ///////////////////////////////////////////////////////////////////////////////////////
    // get the number of spaces in command to differ the arguments
    // spaces numbers = arguments numbers -1
    int spaces_number = 0;
    for(i=0; i<strlen(current_command); i++)
    {
        if(current_command[i]==' ')
        {
            spaces_number++;
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
    // spliting to arguments
    // extra element for NULL
    char *arguments[spaces_number+2];
    i = 0;
    char *p = strtok (current_command, " ");
    while (p != NULL)
    {
        arguments[i++] = p;
        p = strtok (NULL, " ");
    }arguments[i++] = NULL;

    ///////////////////////////////////////////////////////////////////////////////////////

    if(!strcmp(arguments[0], "exit"))
    {
        exit_flag = 1;
        return;
    }
    else if(!strcmp(arguments[0], "cd"))
    {
        if(arguments[1]==NULL || (strcmp(arguments[1], "~")==0) || (strcmp(arguments[1], "~/")==0))
        {
            chdir("/home");
        }
        else if(strcmp(arguments[1], "..")==0)
        {
            chdir("..");
        }
        else if(strcmp(arguments[1], "-")==0)
        {
            chdir("..");
            chdir("..");
        }
        else if(chdir(arguments[1])!=0)
        {
            fprintf(stderr, "error: %s\n", strerror(errno));
        }
        else
        {
            chdir(arguments[1]);
        }
    }
    else
    {
        i=0;
        while (arguments[i] != NULL && background_flag == 0)
        {
            if (strcmp(arguments[i],"&") == 0)
            {
                background_flag = 1;
            }
            i++;
        }
        if(background_flag==0)
        {
            launch_progam(arguments,background_flag);
        }
        else if(background_flag==1)
        {
            launch_progam(arguments,background_flag);
        }
        background_flag = 0;
    }
}

void launch_progam(char **args, int background_flag)
{	 
    pid_t pid;
	 
    if((pid = fork())==-1)
    {
        printf("Child can't launch\n");
        return;
    }
	// pid == 0 implies the following code is related to the child process
	if(pid==0)
    {
        log_file(0,getpid(),1);
        // ignore SIGINT signals (parent process will handle them)	
        signal(SIGINT, SIG_IGN);

        // -1 mean wrong command or error
        if(execvp(args[0], args)==-1)
        {
            printf("wrong command\n");
            // kill(getpid(),SIGTERM);
        }
        // kill the child
        kill(getpid(),SIGTERM);
    }
        
    // This part will happen in the parent
    // To create a forground process wait for the child to finish.
    if (background_flag == 0)
    {
        waitpid(pid,NULL,0);
    }
    else
    {
        // To create a background process don't wait for child.
        log_file(0,getpid(),0);
    }	 
}

void *remove_spaces_and_next_line_at_end(char *command)
{
    input_command[strlen(command)-1] = '\0';
    if(strlen(command)>1)
    {
        int i = strlen(command)-1;

        while(command[i] != '\0' || i==0)
        {
            if(command[i] != ' ')
            {
                break;
            }
            if(command[i] == ' ')
            {
                command[i] = '\0';
            }
            i--;
        }
    }
}

void prompt()
{
	char hostname[max_size] = "";
	gethostname(hostname, sizeof(hostname));
	printf("%s@%s:%s$ ", getenv("LOGNAME"), hostname, getcwd(current_directory, 1024));
}

void intro()
{
    printf("    ===========================================\n");
    printf("    ===== welcome to our shell project ========\n");
    printf("    ===========================================\n");
    printf("    ===========================================\n");
    printf("    ===== We are ready to go, let's start======\n");
    printf("    ===========================================\n\n");
}