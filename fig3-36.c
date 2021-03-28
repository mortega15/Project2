/* Maria Ortega
CPSC 351: Project 2*/
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define HIST_SIZE 10
 
int main (void)
{
	char *args[MAX_LINE/2 + 1];	/* command line (of 80) has max of 40 arguments */
    int should_run = 1; /* flag */

    char userCM[MAX_LINE]; /*user command*/
    int *argv;
    int *isAmp;
    char *hist[HIST_SIZE];
    int length;
    int usePipe = 0;
    char dlm[] = " ";
    int htc = 0;
    int cm = 0;

    pid_t pid;

    while (should_run)
    {
        printf("osh>");
        fflush(stdout);

        /* creating history feature*/

        fgets(&userCM[0], MAX_LINE, stdin);

        userCM[strlen(&userCM[0]) - 1] = '\0';

        if (strcmp("history", &userCM[0]) == 0)
        {
            for (int i = htc - 1; i >= 0; i--)
            {
                printf("\t%d %s\n", i + 1, hist[i]);
            }
            continue;
        }
        length = strlen(&userCM[0]);
        if (userCM[0] == '!' && (length > 1))
        {
            if (userCM[1] == '!')
            {
                strcpy(&userCM[0], hist[htc - 1]);
            }
            else if ((userCM[1] > 47) && (userCM[1] <= 58))
            {
                if (length == 2)
                {
                    cm = userCM[1] - 48;
                }
                else
                {
                    cm = 10 + userCM[2] - 48;
                }

                if ((cm <= htc) && (cm > 0))
                {
                    strcpy(&userCM[0], hist[cm - 1]);
                }
            }
        }

        if (htc < (HIST_SIZE))
        {
            strcpy(hist[htc], &userCM[0]);
            htc++;
            hist[htc] = (char*) malloc(sizeof(char));
        }
        else
        {
            for (int i = 0; i < (HIST_SIZE - 1); i++)
            {
                hist[i] = hist[i + 1];
            }
            strcpy(hist[htc], &userCM[0]);
        }

        /*parse what the user has entered into separate tokens 
        and store tokens in an array of character strings (args)*/
        *argv = 0;
        *isAmp = 0;

        char *token = strtok(userCM, dlm); 
        while (token != NULL) 
        {
            if (token[0] == '&') 
            {
                *isAmp = 1;
                token = strtok(NULL, dlm);
                continue;
            }
        }

        *argv += 1;
        args[*argv - 1] = strdup(token);
        token = strtok(NULL, dlm);
        
        args[*argv] = NULL; /*args array that will be passed to the evecvp() */

        pid = fork(); /* creates child process */

        if (pid == 0) 
        {
            if (argv == 0)
            {
                continue;
            }
            else 
            {
                int rc = 0;
                int file;

                /*Redirecting Input and Output */

                for (int i = 1; i <= *argv-1; i++)
                {
                    if (strcmp(args[i], "<") == 0) /*input redirector operator*/
                    {
                        file = open(args[i+1], O_RDONLY);
                        
                        if (file == -1 || args[i+1] == NULL)
                        {
                            printf("invalid command.\n");
                            exit(1);
                        }

                        dup2(file, STDOUT_FILENO); /*duplicates an existing file descriptor to another file descriptor*/
                        args[i] = NULL;
                        args[i+1] = NULL;
                        rc = 1;
                        break;
                    }

                    else if (strcmp(args[i], ">") == 0) /*output redirection operator*/
                    {
                        file = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                        
                        if (file == -1 || args[i+1]  == NULL)
                        {
                            printf("Invalid command.\n");
                            exit(1);
                        }
                        dup2(file, STDOUT_FILENO);
                        args[i] = NULL;
                        args[i+1] = NULL;
                        rc = 2;
                        break;
                    }

                    /*Communication via a Pipe*/
                    else if (strcmp(args[i], "|") == 0)
                    {
                        usePipe=1;
                        int fd[2];

                        if(pipe(fd) == -1)
                        {
                            fprintf(stderr, "Pipe failed.\n");
                            return 1;
                        }
                        char *fCM[i+1];
                        char *sCM[*argv-i-1+1];

                        for (int j = 0; j < i; j++)
                        {
                            fCM[j] = args[j];
                        }
                        fCM[i] = NULL;

                        for (int j = 0; j < *argv - i - 1; j++)
                        {
                            sCM[j] = args[j + i + 1];
                        }

                        sCM[*argv-i-1] = NULL;
                        
                        /* another child process*/
                        int pipe2 = fork();

                        /* Easier to create separate processes: the parent creates a child process,
                        the child to create another child process that way the child process will 
                        establish a pipe between itself*/

                        if (pipe2 > 0)
                        {
                            wait(NULL);
                            close(fd[1]);
                            dup2(fd[0], STDIN_FILENO);
                            close(fd[0]);
                            if (execvp(sCM[0], sCM) == -1)
                            {
                                printf("Invalid command.\n");
                                return 1;
                            }
                        }
                        else if (pipe2 == 0)
                        {
                            close(fd[0]);
                            dup2(fd[1], STDOUT_FILENO);
                            close(fd[1]);
                            if (execvp(fCM[0], fCM) == -1)
                            {
                                printf("Invalid command.\n");
                                return 1;
                            }
                            exit(1);
                        }
                        close(fd[0]);
                        close(fd[1]);
                        break;                        
                    }
                }
                
                if (rc == 1)
                {
                    close(STDOUT_FILENO);
                }
                
                else if (rc == 2)
                {
                    close(STDOUT_FILENO);   
                }
                close(file);
            }
            
            exit(1);
        }

        else if (pid > 0) /*parent process*/
        {
            if(isAmp == 0)
            {
                wait(NULL); /*parent process waits for chils to exit before it continues*/
                printf("Child complete!\n");
            }
        }

        else /* FAIL */
        {
            fprintf(stderr, "Fork failed!\n");
        }
        

        
        /*After reading user input, the steps are:
        (1) fork a child process
        (2) the child process will invoke execvp()
        (3) if command included &, parent will invoke wait()*/
    }
    
	return 0;
}
