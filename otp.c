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
//	pthread_mutex_t* mutex;
	char* data;
	char* randNums;
	int start;
	int end;
}WorkerArgs;

void* randomNum(void* args){
	RandParameters* params = (RandParameters*)args;
	printf("A = %d , C = %d, M = %d\n", params->a,params->c,params->m);
	params->array[0] = (char)(params->a*params->x+params->c)%params->m;
	for(int i = 1;i < params->size;i++){
		//printf("Random thread\n");
		params->array[i] = (char)(params->a*params->array[i-1]+params->c)%params->m;
		printf("result = %x\n",(char)params->array[i]);
	}
}

void* oneTimePad(void* args){
	WorkerArgs* params = (WorkerArgs*)args;
	printf("OTP\n");
	char buf;
	for(int i = params->start;i < params->end;i++){
		printf("%c",params->data[i]);
		buf = params->data[i] ^ params->randNums[i];
		printf("char = %x, num = %x, xor = %x\n",params->data[i],params->randNums[i],buf);
		params->data[i] = buf;
	}
	pthread_barrier_wait(params->barrier);
}

int main(int argc,char** argv){
	Parameters param;
	FILE* file = NULL;
	int numOfCores = get_nprocs();
	int optVal;
	//getopt(argc,argv,"i");
	printf("%d : num of procs\n", numOfCores);
	while((optVal = getopt(argc,argv,"i:o:x:a:c:m:")) != -1){
		switch(optVal){
			case 'i':
			param.input = optarg;
			//printf("\n");
			break;
			case 'o':
			param.output = optarg;
			break;
			case 'x':
			param.x = atoi(optarg);
			printf("%d\n",param.x);
			break;
			case 'a':
			param.a = atoi(optarg);
			printf("%d\n",param.a);
			break;
			case 'c':
			param.c = atoi(optarg);
			printf("%d\n",param.c);
			break;
			case 'm':
			param.m = atoi(optarg);
			printf("%d\n",param.m);
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
	length--;
	printf("length = %d\n",length);
	fseek(file,0,SEEK_SET);
	char* data = (char*)malloc(length);
	char* array = (char*)malloc(length);
	printf("%ld\n",sizeof(data));
	char c;
	int i = 0;
	while((c = fgetc(file)) != EOF){
		data[i] = c;
		i++;
	}

	fclose(file);
	printf("len of data : %d\n",strlen(data));
	RandParameters randParam = {length,array,param.x,param.a,param.c,param.m};
	pthread_t randThread;
	int create = pthread_create(&randThread,NULL,randomNum,(void*)&randParam);
	if(create != 0){
		printf("Cannot create thread\n");
		return -3;
	}

	pthread_join(randThread,NULL);
	printf("%ld\n",sizeof(array));

	WorkerArgs context[numOfCores];
	pthread_t worker[numOfCores];

	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier,NULL,numOfCores+1);
//	pthread_mutex_t mtx;
//	pthread_mutex_init(&mtx,NULL);

	printf("barrier init()\n");
	for(int i = 0;i < numOfCores;i++){
		printf("start = %d , end = %d\n",i*(int)ceil((float)length/(float)numOfCores),
			(i+1)*(int)ceil((float)length/(float)numOfCores)>length?length:(i+1)*(int)ceil((float)length/(float)numOfCores));
//		context[i].mutex = &mtx;
		context[i].barrier = &barrier;
		context[i].data = data;
		context[i].randNums = array;
		context[i].start = i*(int)ceil((float)length/(float)numOfCores);
		context[i].end = (i+1)*(int)ceil((float)length/(float)numOfCores) > length ? length : (i+1)*(int)ceil((float)length/(float)numOfCores);
		pthread_create(&worker[i],NULL,oneTimePad,(void*)&context[i]);
	}

	pthread_barrier_wait(&barrier);
	pthread_barrier_destroy(&barrier);
	printf("Check\n");
	for(int i = 0;i < length;i++){
		printf("data = %x\n",data[i]);
	}

	file = fopen(param.output,"wb");
	printf("len of data : %d , len of nums : %d\n",strlen(data),strlen(array));
	for(int i = 0;i < length+1;i++){
		fputc(data[i],file);
	}
//	fputc('\0',file);
	fclose(file);


	return 0;
}


