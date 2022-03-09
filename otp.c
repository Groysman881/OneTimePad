#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <getopt.h>
#include <stdlib.h>
#include <math.h>

typedef struct{
  	char* input;
  	char* output;
	int x;
	int a;
	int c;
	int m;
}Parameters;

typedef struct{
	int size;
	char* array;
	int x;
	int a;
	int c;
	int m;
}RandParameters;

typedef struct{
	pthread_barrier_t* barrier;
	char* data;
	char* randNums;
	int start;
	int end;
}WorkerArgs;

void* randomNum(void* args){
	RandParameters* params = (RandParameters*)args;
	params->array[0] = (char)(params->a*params->x+params->c)%params->m;
	for(int i = 1;i < params->size;i++){
		params->array[i] = (char)(params->a*params->array[i-1]+params->c)%params->m;
	}
}

void* oneTimePad(void* args){
	WorkerArgs* params = (WorkerArgs*)args;
	char buf;
	for(int i = params->start;i < params->end;i++){
		buf = params->data[i] ^ params->randNums[i];
		params->data[i] = buf;
	}
	pthread_barrier_wait(params->barrier);
}

int main(int argc,char** argv){
	Parameters param;
	FILE* file = NULL;
	int numOfCores = get_nprocs();
	int optVal;
	while((optVal = getopt(argc,argv,"i:o:x:a:c:m:")) != -1){
		switch(optVal){
			case 'i':
			param.input = optarg;
			break;
			case 'o':
			param.output = optarg;
			break;
			case 'x':
			param.x = atoi(optarg);
			break;
			case 'a':
			param.a = atoi(optarg);
			break;
			case 'c':
			param.c = atoi(optarg);
			break;
			case 'm':
			param.m = atoi(optarg);
			break;
			default:
			printf("Unknown parameter\n");
			return -1;
		}

	}

	if((file = fopen(param.input,"rb")) == NULL){
		printf("Cannot open file\n");
		return -2;
	}

	fseek(file,0,SEEK_END);
	int length = ftell(file);

	if(length > 5000){
		printf("File is too big\n");
		return -4;
	}

	length--;
	fseek(file,0,SEEK_SET);
	char* data = (char*)malloc(length);
	char* array = (char*)malloc(length);
	char c;
	int i = 0;
	while((c = fgetc(file)) != EOF){
		data[i] = c;
		i++;
	}

	fclose(file);
	RandParameters randParam = {length,array,param.x,param.a,param.c,param.m};
	pthread_t randThread;
	int create = pthread_create(&randThread,NULL,randomNum,(void*)&randParam);
	if(create != 0){
		printf("Cannot create thread\n");
		return -3;
	}

	pthread_join(randThread,NULL);

	WorkerArgs context[numOfCores];
	pthread_t worker[numOfCores];

	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier,NULL,numOfCores+1);


	for(int i = 0;i < numOfCores;i++){
		context[i].barrier = &barrier;
		context[i].data = data;
		context[i].randNums = array;
		context[i].start = i*(int)ceil((float)length/(float)numOfCores);
		context[i].end = (i+1)*(int)ceil((float)length/(float)numOfCores) > length ? length : (i+1)*(int)ceil((float)length/(float)numOfCores);
		pthread_create(&worker[i],NULL,oneTimePad,(void*)&context[i]);
	}

	pthread_barrier_wait(&barrier);
	pthread_barrier_destroy(&barrier);

	file = fopen(param.output,"wb");
	for(int i = 0;i < length+1;i++){
		fputc(data[i],file);
	}
	fclose(file);


	return 0;
}


