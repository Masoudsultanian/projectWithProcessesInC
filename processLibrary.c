/*****************************************************************************************
Question 5: write a C program to implement a library borrowing system.
The main process is responsible for maintaining the catalog, including the information 
regarding the bookbeing borrowed or not. It may serve 3 types of requests:
a. To provide a list of library books(all of them).
b. To borrow a book. If the book cannot be borrowed it notifies the temporarry unavailable
of the book.
c. To return book.

Another set of processes are going to simulate a single human interaction with the library
In order to simplify the simulation, each process is behaving following these steps:

1. It requests a list of books.
2. Then it picks a random book among the list and tries to borrow it. If the book is not 
available it waits until it can be borrowed.
3. Once the book has been borrowed, it reads the book for a random number of seconds(up to 
10), then it returns the book.

The steps are repeated for a random number of times(up to 20). Once all processes end the 
program must end too. Implement the final version letting the user define how many human 
processes must be created by providing a number by command line.
******************************************************************************************/
/*Author: Masoud Soltanian. This program is tested for up to 10 processes and 5 books
and it works fine without any particular problem :)
 
Algorithm: Only important points:
1- For each book you have to have a semaphore in order not to permit child processes(customers)
read a book which is lended to another child process.
2- you must know how to create child processes for one parent as much as possible.
3- you need one binary semaphore (token) to only permit one child process communicates with
parent porcess(librarian).
4- In this program a unique pipe is used for communucation between parent and child.
5- Since in processes there is code is exact copy among processes you should find a way to
manage to tell which parat of the code must be run by parent and which part by children.
6- we have to know that each child process copies the code with data version at the time of
its creation. so for example in the program below child[N] array keeps pid of each child but 
child 4 has only process id of 3 children before itself. at the end of child process creation
phase you have to send the final version of pid array to eahc child (done by processList pipe
in this program). 
******************************************************************************************/
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<time.h>
#include<string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>

#define N 10 // number of processes
#define M 5 // number of books
#define SEM_NAME "binarySemaphor1"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 0
typedef struct 
{
    char bookName[M][50];
    //char bookStatus[M];
}booklist;
 
void requestPidInit(int *fd)
{
    if(pipe(fd) == -1)
    {
        fprintf(stderr,"Error in opening the pipe...\n");
		exit(-1); 
    }

}

pid_t waitForChildpid(int *fd)
{
    pid_t childpid;
    if(read(fd[0], &childpid, sizeof(pid_t))==-1)
    {
        fprintf(stderr,"Error in waiting...\n");
		exit(-1);	
    }
    return childpid;
}
void tellMyPidToParent(int *fd, int pid)
{
    if(write(fd[1],&pid,sizeof(int)) == -1)
    {
        fprintf(stderr,"Error in signalinging...\n");
		exit(-1);
    }
}

int main(int argc, char * argv[])
{
    int i;
    char semName[M][10];// 6 character to store name of each book
    sem_t *semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL|O_TRUNC, SEM_PERMS, INITIAL_VALUE);
    sem_t *bookAvailable[M];// M books are available
    for(i=0; i<M; i++)//one semaphore for each book
    {
        sprintf(semName[i], "lib%d", i);
        bookAvailable[i]=sem_open(semName[i],O_CREAT|O_EXCL|O_TRUNC, SEM_PERMS,1);
    }

    int requesterPid[2];
    int parentTochild[N][2];
    int childToParent[N][2];
    int processesList[2];
    pid_t child[N];// to store pid of child processes
    
    
    requestPidInit(requesterPid);//initializing 
    pipe(processesList);
    for(i=0;i<N; i++)//pipe initialization for IPC
    {
        pipe(parentTochild[i]);
        pipe(childToParent[i]);
    }
    
    pid_t parentPid=getpid();
    fprintf(stdout,"I'm the parent process %d \n",parentPid);
    for(i=0;i<N;i++){ //generate only 2 processes + the father is already running = 3
		child[i]=fork();	
		if(child[i]==-1)
        {
			fprintf(stderr,"Error in fork....\n");
			exit(-1);
		}
		else
        {
			if(child[i]==0)
            {
				fprintf(stdout,"child created %d and my parent is %d\n",getpid(),getppid());
				break;//The child exits from the cycle of generation
			}
			//The father continues to cycle to generate the other children
		}
	}
    if(getpid() == parentPid)//Parent process (librarian)
    {
        close(processesList[0]);
        for(i=0; i<N; i++)
        {
            write(processesList[1],child, sizeof(pid_t)*N);//parent writes 3 time on common pipe the list all processes 
        }                                                   //for each child in order to find themselves in the list
        
        close(processesList[1]);
        int requestedBookIndex=0;
        booklist libraryBooks;
        strcpy(libraryBooks.bookName[0],"Cprogramming");
        strcpy(libraryBooks.bookName[1],"C++programming");
        strcpy(libraryBooks.bookName[2],"C#programming");
        strcpy(libraryBooks.bookName[3],"PythonProgramming");
        strcpy(libraryBooks.bookName[4],"JavaProgramming");
        
  
        for(i=0; i<N; i++)
        {
            close(parentTochild[i][0]);//closing read from parent to child pipe
            close(childToParent[i][1]);//closing write on child to parent pipe
        }

        pid_t childpid=0;
        int childId=0;

        sem_post(semaphore);
        printf("parant released the token\n");
        for (i=0; i<N; i++)
        {
            printf("parent is waiting for a child who is going to get token, iteration: %d \n",i);
            childpid = waitForChildpid(requesterPid);
            printf("parent: child %d got the token\n", childpid);
            for(int j=0; j<N; j++)//search to find the child who has sent the request to send the list of books on corresponding pipe
            {
                if(childpid == child[j])
                {
                    childId=j;
                    printf("found index [%d] pid: [%d]\n",j,child[j]);
                }
            }
            write(parentTochild[childId][1],&libraryBooks, sizeof(booklist));
            read(childToParent[childId][0], &requestedBookIndex, sizeof(requestedBookIndex));
            printf("requested book index from child [%d] is: %d and the book is [%s]\n",childpid, requestedBookIndex, libraryBooks.bookName[requestedBookIndex]);
        }
        sem_unlink(SEM_NAME);
        sem_destroy(semaphore);
        for(i=0;i<M;i++)
        {
            sem_unlink(semName[i]);
            sem_destroy(bookAvailable[i]);
        }
        
        for(i=0;i<N;i++)
        {
			wait(NULL);//I wait for the death of all the children in order to avoid zombies
		}
        printf("PARENT SAYS GOODBYE\n");
        return 0;

    }
    else//childs (humans asking for book)// for the moment 2 processes are active on this else
    {
        sleep(1);
        pid_t processes[N];
        close(requesterPid[0]);
        close(processesList[1]);
        read(processesList[0],processes, sizeof(pid_t)*N);
        close(processesList[1]);
        // time_t t;
        // srand(time(&t));
        booklist books;
        for(i=0; i<N; i++)
        {
            close(parentTochild[i][1]);//closing write on parent to child pipe
            close(childToParent[i][0]);//closing read from child to parent pipe
        }
        pid_t myPid=getpid();
        srand(myPid);
        // for(int j=0; j<N; j++)//closing all pipes except mine.
        // {
        //     if(myPid == processes[j])
        //     {
        //         // close(parentTochild[j][1]);//closing write on parent to child pipe
        //         // close(childToParent[j][0]);//closing read from child to parent pipe
        //         continue;
        //     }
        //     close(parentTochild[j][0]);
        //     close(parentTochild[j][1]);
        //     close(childToParent[j][0]);
        //     close(childToParent[j][1]);
        // }
        int myId=0;//index of my pid in the parent child[] array that keeps all the childs' pids
        for(int j=0; j<N; j++)//child: To find who I am.
        {
            //printf("[%d] jadval pid:%d \n",myPid,child[j]);
            printf("[%d] processkhodam pid:%d \n",myPid,processes[j]);
            if(myPid == processes[j])
            {
                myId=j;
            }
        }
        pid_t ppid=0;
        printf("I am child [%d] going to wait for token\n", getpid());
            
        sem_wait(semaphore);
    
        tellMyPidToParent(requesterPid,myPid);
        printf("child [%d] could send its request now waits for list of books\n", myPid);

        read(parentTochild[myId][0],&books,sizeof(booklist));
        printf("child [%d] could get the book list\n",myPid);
        int randomBook= rand()%M;//M books
        write(childToParent[myId][1], &randomBook, sizeof(int));

        sem_post(semaphore);// relese the token to let other processes start to borrow book

        printf("child [%d] waits for book number [%d] to become available!!! \n",myPid,randomBook);
        sem_wait(bookAvailable[randomBook]);
        printf("child [%d] could borrow book number [%d] !!! \n",myPid, randomBook);
        sleep(5);//five second
        printf("child [%d] finished reading book number [%d]\n", myPid, randomBook);
        sem_post(bookAvailable[randomBook]);
        printf("child [%d] says Goodbye!!!\n", myPid);
        exit(0);
    }
    
}