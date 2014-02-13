/*
gcc vconfserver.c surulib.c -o server
gcc vconfserver.c surulib.c -o server -g
*/

/*
STEP I
========================
telnet 127.0.0.1 1980

STEP II
=========================
{"command":"register_rtsp_address","username":"daserfost","rtspuri":"ssss"}

STEP III (Depends on the Outcome of STEP II
==============================
{"command":"list_of_clients","sid":"568895"}
*/

/* Standard C Library + POSIX Library*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <sys/shm.h>

/* Json Parser Library By: Copyright (2012) Daser Retnan @ https://github.com/retnan/surulib/  	*/
#include "surulib.h"

#define PORT "1980"  // the port users will be connecting to

#define IP "127.0.0.1"

#define BACKLOG 100     // how many pending connections queue will hold

#define MAX_BYTE 100     //Max Number of bytes received from client

#define FILELOG	"blessing.log"

#define LOCK_FILE "blessing.lock"

#define SIZE 256

#define MAXU 150

typedef struct registered{
char rtsp[MAXU];
char name[MAXU];
int sid;
}registry;

void sigchld_handler(int s);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

void initialize_server(void);

void write_to_log(char * message);

int is_client_registered(const char * sid, const registry * uulist, int n);

int main(void)
{

	int fd;
	
	pid_t child_pid;
	child_pid = fork ();
	if (child_pid != 0) {
	//parent ID
	exit(0); //the parent needs to die so the child can leave
	}else{
	//child process executing
	setsid();
	for (child_pid=getdtablesize();child_pid>=0;--child_pid) close(child_pid);
		
			fd = open(LOCK_FILE,O_RDWR|O_CREAT,0777);
			
			if (fd<0){
			write_to_log(" :Fatal Error: couldnot read/create lock file. permission reasons");
			exit(1); /* can not open */
			}
		
			if (lockf(fd,F_TLOCK,0) < 0){
			write_to_log(" :Another Instance of Video Server is already running\n");
			fprintf(stderr,"See *.log for details\n");
			exit(0); /* can not lock */
			}
		
		initialize_server();
	}

return 0;
}
	
void write_to_log(char * message){
char buffer[SIZE];
	time_t curtime;
	struct tm *loctime;
	curtime = time (NULL);
	loctime = localtime (&curtime);
	FILE *fp;
	fp =fopen(FILELOG,"a+");

			if(fp == NULL){
			perror("Could not open\n");
			}

	strcpy(buffer,asctime(loctime));
	strcat(buffer,message);
	fprintf(fp,"%s\n",buffer);
	fclose(fp);
}

void initialize_server(void){
	
    fprintf(stdout,"Video Conf Server starting....\n");
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
	int cmesg_lent = 0;
	char rmesg[MAX_BYTE];
    int rv;

    database *parsed_json = NULL;

/* data structure for users */
    char command[100];
    char name[100];
    char rtsp[100];
    char sid[100];

/* used for random number generation to generate session id*/
    struct timeval tv;
    unsigned low = 100000;
    unsigned high = 999999;
    unsigned range = high - low + 1;
    int entropy;

/* response buffer */
   char recipt[100];
   char recipt2[1000];

    registry *userstruct =  NULL;
    registry *data = NULL;

    memset(&hints, 0, sizeof(struct sockaddr_in) );
	
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
	hints.ai_canonname = (char *)"VideoConf";
	hints.ai_next = NULL;

    if ((rv = getaddrinfo(IP, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }
	
	p = servinfo;
	
    if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
       perror("server: socket");
     }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
     }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
     }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
	
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }


    sin_size = sizeof(struct sockaddr);

    /* create a shared memory for IPC */ 
    int data_seg_id, mct_id;

    data_seg_id = shmget (1981, getpagesize(),IPC_CREAT | 0666);	// This store the struct
    userstruct = (registry*) shmat (data_seg_id, NULL, 0);//RW	//this page store glist
    mct_id = shmget (1982, sizeof(int),IPC_CREAT | 0666);	// This store the struct
    int * mcount = (int*) shmat (mct_id, NULL, 0);//RW	//this page store glist
    *mcount = 0;

    printf("Blessing's Server: waiting for connections from Video Conferencing Clients...\n");

    while(1) {  // main accept() loop
		
 		    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		
		    if (new_fd == -1) {
		        perror("accept");
		        continue;
		    }

		    inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof(s) );

		    if (!fork()) { // this is the child process
		        close(sockfd); // child doesn't need the listener
		        setsid();
				bzero(rmesg,MAX_BYTE - 1);

				cmesg_lent = read(new_fd, rmesg, MAX_BYTE - 1);

				if(cmesg_lent == 0){
					write_to_log(" : Captured an empty message, PING i guess\n");
					goto close;
				}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
parsed_json = (database*)SURU_initJson(rmesg);

if(!parsed_json){
write_to_log("This client doesn't understand server protocol\n");
goto close;
}

bzero(command,sizeof(char) - 1);	/* Nullify buffer */
strcpy(command,(char *)SURU_getElementValue(parsed_json,"command"));

if(strcmp(command,"register_rtsp_address") == 0){

/* data_seg_id = shmget (1981, getpagesize(),IPC_CREAT | 0666);	// This store the struct */
	userstruct = (registry*) shmat (data_seg_id, NULL, 0);//RW	//inherite data_seg_id
/* mct_id = shmget (1982, sizeof(int),IPC_CREAT | 0666);	// This store the struct */
	mcount = (int*) shmat (mct_id, NULL, 0);//RW	//mct_id was inherited

/* generate session id for registed client */
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec);	/*FIX ME 'cos this is not a secure way but no time :-) */
	entropy = low + (int) (((double) range) * rand () / (RAND_MAX + 1.0)); /* random number */

/* hmmn! No validation. Might SegFault. Murphy's law */
	strcpy(rtsp,(char *)SURU_getElementValue(parsed_json,"rtspuri")); 
	strcpy(name,(char *)SURU_getElementValue(parsed_json,"username"));
	
	strcpy(userstruct[*mcount].rtsp,(char *)rtsp);
	strcpy(userstruct[*mcount].name,(char *)name);
	userstruct[*mcount].sid = entropy;

	*mcount = *mcount + 1;
/* Detatach shared segment from this process VM space */
	shmdt (mcount);
	shmdt (userstruct);

/* response text */
	bzero(recipt,sizeof(recipt) - 1);
	sprintf(recipt, "{\"command\":\"register\",\"sid\":\"%d\"}", entropy);
	send(new_fd, recipt, strlen(recipt), 0);	// assuming the sizeof char is 1byte
	goto close;
}else
if(strcmp(command,"list_of_clients") == 0){

strcpy(sid,(char *)SURU_getElementValue(parsed_json,"sid"));


registry * uulist = (registry*) shmat (data_seg_id, NULL, 0);//RW	//inherite data_seg_id
mcount = (int*) shmat (mct_id, NULL, 0);//RW	//mct_id was inherited

	if(is_client_registered(sid,uulist,*mcount)){

		bzero(recipt,sizeof(recipt) - 1);
		strcpy(recipt,"Online Users:");

		int n = 0;

			while (n < *mcount) {
			  strcat(recipt, "Username=");
		    	  strcat(recipt, (char*)uulist[n].name);
			  strcat(recipt, ":RTSP=");
			  strcat(recipt, (char*)uulist[n].rtsp);
			  strcat(recipt, "\n");
			  n++;
			}//WEND
	bzero(recipt2,sizeof(recipt2) - 1);
	sprintf(recipt2, "{\"command\":\"list_of_clients\",\"result\":\"%s\"}", recipt); /* Dangerous */
	send(new_fd, recipt2, strlen(recipt2), 0);	//We asssume here that the sizeof char is 1byte
        bzero(recipt,sizeof(recipt) - 1);
	write_to_log("Server Responded to Client's request\n");

/* Detatach shared segment from this process VM space */
	shmdt (mcount);
	shmdt (userstruct);

	goto close;

	}else{
	send(new_fd, "{\"command\":\"error\",\"reason\":\"Register RTSP First\"}", 51, 0);
	write_to_log("Client has not registered hence, cannot get a list: register your rtsp URI and get an sid\n");
	goto close;
	}

}else{
send(new_fd, "{\"command\":\"error\",\"reason\":\"Invalid Command\"}", 47, 0);
write_to_log("Invalid Command\n");
goto close;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////	
			SURU_free(parsed_json);
			close:
		        close(new_fd);
				close(getpid());
		        exit(0);
		    }else{
			//parenting parole
		    }
		    close(new_fd);  // parent doesn't need this
    }

    return;

}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int is_client_registered(const char * chsid, const registry * uulist, int n)
{
//DEBUGING
//return 1;
//DEBUGING

registry* data = NULL;
int sid = atoi(chsid);
int c = 0;

	for(; c < n; c++){
/* Search for a SID */
            if( (int)uulist[c].sid == sid){		// This will segfault if shared memory is wongly setup
                return 1;
                break;
            }
        }
return 0;
}
