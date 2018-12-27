#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <aio.h>
#include <errno.h>
#include<pthread.h>
#include <semaphore.h>
#include <time.h>
			
#define SOURCE_FILE "source.txt"
#define DEST_FILE "dest.txt"

	void *copyPaste(void *arg);			//Method for each thread to copy paste
	void createRandFile(size_t length,char *path);	
	void printProgBar(int percetage);  		//Method for printing Progress Bar

	int size; 				//Variable for size of randomly created file
	int count = 0;			//Variables and semaphore to control thread number
	int completeCount = 0;		//integer to find percentage of all completed paste
	int threadCount;

	sem_t incCount;			//Required Semaphores
	sem_t compControll;


	int main(int argc, char *argv[])
	{
	   // argv[1]="/home/kerim/Desktop/AsyncReadWrite";		//Args Template
	   // argv[2]="/home/kerim/Desktop/AsyncReadWrite";
	   // argv[3]= "10";
		
		if(argv[1]==NULL || argv[2]==NULL || argv[3]==NULL){
			printf("Error! Enter path of files and thread count.\n");
			exit(1);
		}
		sem_init(&incCount,0,1);							//semaphore initilazing 
		sem_init(&compControll,0,1);
		FILE *fp;
		char fileName[100];
		if(strcmp(argv[2],"-")==0){			//Open close file for a clear
			fp = fopen(DEST_FILE,"w");
		}
		else{
			snprintf(fileName,strlen(argv[2]) + strlen(DEST_FILE) + 2,"%s/%s",argv[2],DEST_FILE);
			fp = fopen(fileName,"w");
		}		
		fclose(fp);
		threadCount = strtol(argv[3], NULL, 10);	//taking thread count grom args 
	  	if(threadCount>10 || threadCount < 0){
			printf("Thread Count must be 0-10\n");
			exit(1);
	
		}
	  	srand(time(NULL));     
		size = rand() % 104857600;//Randomly sized file max 100 mb
		
		if(argv[4]!=NULL){	//You can enter size of file from console 
			size=strtol(argv[4], NULL, 10);
		}
		if(strcmp(argv[1],"-")==0)
			createRandFile(size,SOURCE_FILE);

		else{
			snprintf(fileName,strlen(argv[1]) + strlen(SOURCE_FILE) + 2 ,"%s/%s",argv[1],SOURCE_FILE);
			createRandFile(size,fileName);
		}						
	   	
		pthread_t threads[threadCount];
		int i;
		printf("\nStarted copying...\n");
		for(i = 0; i< threadCount ; i++)					//Creating threads
			pthread_create(&threads[i],NULL,copyPaste,(void *) argv);
		
		
		 for(i = 0; i< threadCount ; i++)
		 	pthread_join(threads[i],NULL);
		 
	   
		 printf("\n\e[?25hCopy processing completed.\n");
		    // \e[?25h  returns cursor
		return EXIT_SUCCESS;
	}



	void createRandFile(size_t length,char *path) {			
				//This function creates a random text and writes into a file
	
		FILE *fp;
		int n;
		printf("Generating a random file of %d bytes.\n",length);
		static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,.-#'?!";        
		char *randomText = NULL;

		if (length) {
		    randomText = malloc(sizeof(char) * (length +1));
			printProgBar(0);
		    if (randomText) {            
		        for (n = 0;n < length;n++) {            
		            int key = rand() % (int)(sizeof(charset) -1);
		         
		         	if(n==length/2)
		         		printProgBar(50);
		            	
		            randomText[n] = charset[key];
		        }

		        randomText[length] = '\0';
		    }
		    printProgBar(100);
		}

		fp = fopen(path,"w");
		if(fp == NULL){
		 printf("\nError!\n");   
		  exit(1); 
		}
		fprintf(fp,"%s",randomText);
		fclose(fp);

	}

	void *copyPaste(void *arg){
		int source,dest;    			//Pointer for files
		char **arguments = (char**)arg; 				
		 
		sem_wait(&incCount);			//Declaring each threads number
		int threadNo = count;
		count++;
		sem_post(&incCount);
	
		int err,ret,copy_size;				//Required variables
		struct aiocb aio_r,aio_w;
		
		copy_size = size / threadCount;			//copy size for all threads 
		
		if(size % threadCount != 0 && threadNo == threadCount-1){
		//If last thread, copy till the end of file
			copy_size = copy_size + (size % threadCount);
		}
		char *data;
		data = (char *)malloc(copy_size);
		char fileName[100];
		if(strcmp(arguments[1],"-")==0)
			source = open(SOURCE_FILE,O_RDONLY);	//Reading from randomly created file
		
		else{			
			snprintf(fileName,strlen(arguments[1]) + strlen(SOURCE_FILE) + 2 ,"%s/%s",arguments[1],SOURCE_FILE);
			//printf("offset %3d | copy size %3d\n",threadNo * (size / threadCount),copy_size);	
			source = open(fileName,O_RDONLY);
		}
		if (source == -1) {
		    perror("open");
		    exit(2);
		}
	
		memset(&aio_r, 0, sizeof(aio_r));
		
		aio_r.aio_lio_opcode = LIO_READ;
		aio_r.aio_fildes = source; 
		aio_r.aio_buf = data;
		aio_r.aio_offset = threadNo * (size / threadCount);
		aio_r.aio_nbytes = copy_size;
		aio_read(&aio_r);
		while (aio_error(&aio_r) == EINPROGRESS) { }
		err = aio_error(&aio_r);
		ret = aio_return(&aio_r);

		if (err != 0) {
		    printf ("Error at aio_error()R %d: %s\n",threadNo+1, strerror (err));
		    close (source);
		    exit(2);
		}

		if (ret != copy_size) {
		    printf("Error at aio_return()R %d\n",threadNo+1);
		    close(source);
		    exit(2);
		}

		close(source);
	  
	 	// printf ("Reading is finished by thread %d\n",threadNo+1);
		if(strcmp(arguments[2],"-")==0){
			dest = open (DEST_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		}
		else{
			snprintf(fileName,strlen(arguments[2]) + strlen(DEST_FILE) + 2,"%s/%s",arguments[2],DEST_FILE);	
			dest = open (fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		}
		
		
		if (dest == -1) {
		    perror("open");
		    exit(1);
		}
					//Writing into file
		memset(&aio_w, 0, sizeof(aio_w));
		aio_w.aio_fildes = dest;
		aio_w.aio_buf = data;
		aio_w.aio_nbytes = copy_size;
		aio_w.aio_offset = threadNo * (size / threadCount);
		if (aio_write(&aio_w) == -1) {
		    printf(" Error at aio_write()W: %s\n", strerror(errno));
		    close(dest);
		    exit(2);
		}
		while (aio_error(&aio_w) == EINPROGRESS) { }
		err = aio_error(&aio_w);
		ret = aio_return(&aio_w);

		if (err != 0) {
		    printf ("Error at aio_error()W %d: %s\n", threadNo+1,strerror (err));
		    close (dest);
		    exit(2);
		}

		if (ret != copy_size) {
		    printf("Error at aio_return()W %d\n",threadNo+1);
		    close(dest);
		    exit(2);
		}
		
		close(dest);
		
		sem_wait(&compControll);
	  	completeCount++;								
	  //printf ("Writing is finished by thread %d\n",threadNo+1);
		int percentage = completeCount*100/threadCount;  
	   	printProgBar(percentage);   					
		usleep(300 * 1000);	//sleep 300 ms for visible progress bar	
		sem_post(&compControll);
	
	}
	
	void printProgBar(int percentage){
		int i;
		printf("\r[");
    	for(i=0;i < percentage/2;i++){
			printf("=");
			fflush(stdout);
		}
		printf(">");
		fflush(stdout);
		for( i=percentage/2;i < 50 ;i++){
			printf(" ");
			fflush(stdout);
		}
		printf("] %d%%\e[?25l",percentage);				// \e[?25l removes the cursor from the terminal
		fflush(stdout);
	
	} 