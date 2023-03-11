/*
Developed by Luis Alberto
email: alberto.bsd@gmail.com
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <semaphore.h>
#include "util.h"

#include "gmpecc.h"
#include "base58/libbase58.h"
#include "rmd160/rmd160.h"
#include "sha256/sha256.h"
#include "threadpool.h"

const char *version = "0.1.20210918";
const char *EC_constant_N = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";
const char *EC_constant_P = "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f";
const char *EC_constant_Gx = "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798";
const char *EC_constant_Gy = "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8";


const char *formats[3] = {"publickey","rmd160","address"};
const char *looks[2] = {"compress","uncompress"};

void showhelp();
void set_format(char *param);
void set_look(char *param);
void set_bit(char *param);
void set_publickey(char *param);
void set_range(char *param);
void generate_straddress(struct Point *publickey,bool compress,char *dst);
void generate_strrmd160(struct Point *publickey,bool compress,char *dst);
void generate_strpublickey(struct Point *publickey,bool compress,char *dst);

typedef struct ThreadArgsMain_ {
	char str_publickey[131];
	char str_rmd160[41];
	char str_address[41];
	char str_publickey_1[131];
	char str_rmd160_1[41];
	char str_address_1[41];
	struct Point target_publickey;
       	struct Point base_publickey;
	struct Point sum_publickey;
	struct Point negated_publickey;
	struct Point dst_publickey;
	pthread_mutex_t* pMutex;
	sem_t* pSemaphore;
	FILE* pOutput;
	mpz_t base_key, sum_key, dst_key;
}ThreadArgsMain;

struct Point base_publickey,sum_publickey,negated_publickey,dst_publickey;
struct Point target_publickey;

int FLAG_RANGE = 0;
int FLAG_BIT = 0;
int FLAG_RANDOM = 0;
int FLAG_PUBLIC = 0;
int FLAG_FORMART = 0;
int FLAG_HIDECOMMENT = 0;
int FLAG_LOOK = 0;
int FLAG_MODE = 0;
int FLAG_N;
uint64_t N = 0,M;

mpz_t min_range,max_range,diff,TWO,base_key,sum_key,dst_key;
gmp_randstate_t state;

void taskOne(void* pArg)
{
	ThreadArgsMain* pMainArgs = pArg;
        FILE* OUTPUT = pMainArgs->pOutput;	

	mpz_urandomm(pMainArgs->base_key,state,diff);
        Scalar_Multiplication(G,&(pMainArgs->base_publickey), pMainArgs->base_key);
        Point_Negation(&(pMainArgs->base_publickey),&(pMainArgs->negated_publickey));
        Point_Addition(&(pMainArgs->base_publickey),&(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));

        switch(FLAG_FORMART)    
	{
                 case 0: //Publickey
                        generate_strpublickey(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_publickey);

                        Point_Addition(&(pMainArgs->negated_publickey),&(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
                        generate_strpublickey(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_publickey_1);
                        if(FLAG_HIDECOMMENT)   
		       	{
				pthread_mutex_lock(pMainArgs->pMutex);
                                fprintf(OUTPUT,"%s\n", pMainArgs->str_publickey);
                                fprintf(OUTPUT,"%s\n", pMainArgs->str_publickey_1);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        else    
			{
				pthread_mutex_lock(pMainArgs->pMutex);
                                gmp_fprintf(OUTPUT,"%s # - %Zd\n",pMainArgs->str_publickey, pMainArgs->base_key);
                                gmp_fprintf(OUTPUT,"%s # + %Zd\n",pMainArgs->str_publickey_1, pMainArgs->base_key);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        break;
		 case 1: //rmd160
                        generate_strrmd160(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_rmd160);
                        
			Point_Addition(&(pMainArgs->negated_publickey),&(pMainArgs->target_publickey),&(pMainArgs->dst_publickey));
                        generate_strrmd160(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_rmd160_1);
                        if(FLAG_HIDECOMMENT)    
			{
				pthread_mutex_lock(pMainArgs->pMutex);
                                fprintf(OUTPUT,"%s\n", pMainArgs->str_rmd160);
                                fprintf(OUTPUT,"%s\n", pMainArgs->str_rmd160_1);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        else    
			{
				pthread_mutex_lock(pMainArgs->pMutex);
                                gmp_fprintf(OUTPUT,"%s # - %Zd\n", pMainArgs->str_rmd160, pMainArgs->base_key);
                                gmp_fprintf(OUTPUT,"%s # + %Zd\n", pMainArgs->str_rmd160_1, pMainArgs->base_key);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        break;
		 case 2: //address
                        generate_straddress(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_address);
                        
			Point_Addition(&(pMainArgs->negated_publickey),&(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
                        generate_straddress(&(pMainArgs->dst_publickey),FLAG_LOOK == 0,pMainArgs->str_address_1);
                        if(FLAG_HIDECOMMENT)   
		       	{
				pthread_mutex_lock(pMainArgs->pMutex);
                                fprintf(OUTPUT,"%s\n", pMainArgs->str_address);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        else    
			{
				pthread_mutex_lock(pMainArgs->pMutex);
                                gmp_fprintf(OUTPUT,"%s # - %Zd\n", pMainArgs->str_address, pMainArgs->base_key);
                                gmp_fprintf(OUTPUT,"%s # + %Zd\n", pMainArgs->str_address_1, pMainArgs->base_key);
				pthread_mutex_unlock(pMainArgs->pMutex);
                        }
                        break;
          }

	sem_post(pMainArgs->pSemaphore);
      	mpz_clear(pMainArgs->base_publickey.x);
      	mpz_clear(pMainArgs->base_publickey.y);
      	mpz_clear(pMainArgs->sum_publickey.x);
      	mpz_clear(pMainArgs->sum_publickey.y);
      	mpz_clear(pMainArgs->negated_publickey.x);
      	mpz_clear(pMainArgs->negated_publickey.y);
      	mpz_clear(pMainArgs->dst_publickey.x);
      	mpz_clear(pMainArgs->dst_publickey.y);
      	mpz_clear(pMainArgs->target_publickey.x);
      	mpz_clear(pMainArgs->target_publickey.y);
	mpz_clear(pMainArgs->base_key);
	mpz_clear(pMainArgs->sum_key);

        //printf("freeing taskone args %p\n", pArg);

	pMainArgs->pMutex = NULL;
	pMainArgs->pSemaphore = NULL;
	pMainArgs->pOutput = NULL;
	free(pMainArgs);
}

void taskTwo(void* pArgs) 
{
	ThreadArgsMain* pMainArgs = pArgs;
        FILE* OUTPUT = pMainArgs->pOutput;	

	Point_Negation(&(pMainArgs->sum_publickey),&(pMainArgs->negated_publickey));
	Point_Addition(&(pMainArgs->sum_publickey),&(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
	switch(FLAG_FORMART)
	{
         	case 0: //Publickey
                      generate_strpublickey(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_publickey);

                      Point_Addition(&(pMainArgs->negated_publickey), &(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
                      generate_strpublickey(&(pMainArgs->dst_publickey), FLAG_LOOK == 0, pMainArgs->str_publickey_1);
                      if(FLAG_HIDECOMMENT)    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_publickey);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_publickey_1);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      else    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            gmp_fprintf(OUTPUT,"%s # - %Zd\n", pMainArgs->str_publickey, pMainArgs->sum_key);
                            gmp_fprintf(OUTPUT,"%s # + %Zd\n", pMainArgs->str_publickey_1, pMainArgs->sum_key);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      break;
		case 1: //rmd160
                      generate_strrmd160(&(pMainArgs->dst_publickey),FLAG_LOOK == 0, pMainArgs->str_rmd160);
                      
		      Point_Addition(&(pMainArgs->negated_publickey), &(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
                      generate_strrmd160(&(pMainArgs->dst_publickey), FLAG_LOOK == 0, pMainArgs->str_rmd160_1);

                      if(FLAG_HIDECOMMENT)    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_rmd160);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_rmd160_1);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      else    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            gmp_fprintf(OUTPUT,"%s # - %Zd\n", pMainArgs->str_rmd160, pMainArgs->sum_key);
                            gmp_fprintf(OUTPUT,"%s # + %Zd\n", pMainArgs->str_rmd160_1, pMainArgs->sum_key);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      break;
		case 2: //address
                      generate_straddress(&(pMainArgs->dst_publickey), FLAG_LOOK == 0, pMainArgs->str_address);
                      
		      Point_Addition(&(pMainArgs->negated_publickey), &(pMainArgs->target_publickey), &(pMainArgs->dst_publickey));
                      generate_straddress(&(pMainArgs->dst_publickey), FLAG_LOOK == 0, pMainArgs->str_address_1);
                      if(FLAG_HIDECOMMENT)    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_address);
                            fprintf(OUTPUT,"%s\n", pMainArgs->str_address_1);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      else    
		      {
			    pthread_mutex_lock(pMainArgs->pMutex);
                            gmp_fprintf(OUTPUT,"%s # - %Zd\n", pMainArgs->str_address, pMainArgs->sum_key);
                            gmp_fprintf(OUTPUT,"%s # + %Zd\n", pMainArgs->str_address_1, pMainArgs->sum_key);
			    pthread_mutex_unlock(pMainArgs->pMutex);
                      }
                      break;
            }

      
      sem_post(pMainArgs->pSemaphore);
      
      mpz_clear(pMainArgs->base_publickey.x);
      mpz_clear(pMainArgs->base_publickey.y);
      mpz_clear(pMainArgs->sum_publickey.x);
      mpz_clear(pMainArgs->sum_publickey.y);
      mpz_clear(pMainArgs->negated_publickey.x);
      mpz_clear(pMainArgs->negated_publickey.y);
      mpz_clear(pMainArgs->dst_publickey.x);
      mpz_clear(pMainArgs->dst_publickey.y);
      mpz_clear(pMainArgs->target_publickey.x);
      mpz_clear(pMainArgs->target_publickey.y);
      mpz_clear(pMainArgs->base_key);
      mpz_clear(pMainArgs->sum_key);

      //printf("%ld freeing tasktwo args %p\n", pthread_self(), pArgs);
      pMainArgs->pMutex = NULL;
      pMainArgs->pSemaphore = NULL;
      pMainArgs->pOutput = NULL;
      //printf("%ld deallocating str_publickey %p\n", pthread_self(), pMainArgs->str_publickey);
      //printf("%ld deallocating str_rmd160 %p\n", pthread_self(), pMainArgs->str_rmd160);
      //printf("%ld deallocating str_address %p\n", pthread_self(), pMainArgs->str_address);
      //printf("%ld deallocating target_publickey %p\n", pthread_self(), &(pMainArgs->target_publickey));
      //printf("%ld deallocating base_publickey %p\n", pthread_self(), &(pMainArgs->base_publickey));
      //printf("%ld deallocating sum_publickey %p\n", pthread_self(), &(pMainArgs->sum_publickey));
      //printf("%ld deallocating negated_publickey %p\n", pthread_self(), &(pMainArgs->negated_publickey));
      //printf("%ld deallocating dst_publickey %p\n", pthread_self(), &(pMainArgs->dst_publickey));
      //printf("%ld deallocating pMutex %p\n", pthread_self(), pMainArgs->pMutex);
      //printf("%ld deallocating pSemaphore %p\n", pthread_self(), pMainArgs->pSemaphore);
      //printf("%ld deallocating pThreadPool %p\n", pthread_self(), pMainArgs->pThreadPool);
      //printf("%ld deallocating pOutput %p\n", pthread_self(), pMainArgs->pOutput);
      
      //free(pMainArgs);
}

int main(int argc, char **argv)  {
	FILE *OUTPUT;
	char c;
	uint64_t i = 0;
	char *str_output;
	char str_publickey[131];
	char str_rmd160[41];
	char str_address[41];

	mpz_init_set_str(EC.p, EC_constant_P, 16);
	mpz_init_set_str(EC.n, EC_constant_N, 16);
	mpz_init_set_str(G.x , EC_constant_Gx, 16);
	mpz_init_set_str(G.y , EC_constant_Gy, 16);
	init_doublingG(&G);

	mpz_init(min_range);
	mpz_init(max_range);
	mpz_init(diff);
	mpz_init_set_ui(TWO,2);
	mpz_init(target_publickey.x);
	mpz_init_set_ui(target_publickey.y,0);
	while ((c = getopt(argc, argv, "hvxRb:n:o:p:r:f:l:")) != -1) {
		switch(c) {
			case 'x':
				FLAG_HIDECOMMENT = 1;
			break;
			case 'h':
				showhelp();
				exit(0);
			break;
			case 'b':
				set_bit((char *)optarg);
				FLAG_BIT = 1;
			break;
			case 'n':
				N = strtol((char *)optarg,NULL,10);
				if(N<= 0)	{
					fprintf(stderr,"[E] invalid bit N number %s\n",optarg);
					exit(0);
				}
				FLAG_N = 1;
			break;
			case 'o':
				str_output = (char *)optarg;
			break;
			case 'p':
				set_publickey((char *)optarg);
				FLAG_PUBLIC = 1;
			break;
			case 'r':
				set_range((char *)optarg);
				FLAG_RANGE = 1;
			break;
			case 'R':
				FLAG_RANDOM = 1;
			break;
			case 'v':
				printf("version %s\n",version);
				exit(0);
			break;
			case 'l':
				set_look((char *)optarg);
			break;
			case 'f':
				set_format((char *)optarg);
			break;
		}
	}

	ThreadPool* pThreadPool = initThreadPool(true);
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	//printf("FLAG_BIT %d, FLAG_RANGE %d, FLAG_PUBLIC %d, FLAG_N %d\n", FLAG_BIT, FLAG_RANGE, FLAG_PUBLIC, FLAG_N);
	if((FLAG_BIT || FLAG_RANGE) && FLAG_PUBLIC && FLAG_N)	{
		if(str_output)	{
			OUTPUT = fopen(str_output,"a");
			if(OUTPUT == NULL)	{
				fprintf(stderr,"can't opent file %s\n",str_output);
				OUTPUT = stdout;
			}
		}
		else	{
			OUTPUT = stdout;
		}
		if(N % 2 == 1)	{
			N++;
		}
		M = N /2;
		mpz_sub(diff,max_range,min_range);
		mpz_init(base_publickey.x);
		mpz_init(base_publickey.y);
		mpz_init(sum_publickey.x);
		mpz_init(sum_publickey.y);
		mpz_init(negated_publickey.x);
		mpz_init(negated_publickey.y);
		mpz_init(dst_publickey.x);
		mpz_init(dst_publickey.y);
		mpz_init(base_key);
		mpz_init(sum_key);
	
		sem_t semaphore;
		
	        //printf("FLAG_RANDOM %d\n", FLAG_RANDOM);
		if(FLAG_RANDOM)	{
			sem_init(&semaphore, 0, 2-M);
			gmp_randinit_mt(state);
			gmp_randseed_ui(state, ((int)clock()) + ((int)time(NULL)) );
			for(i = 0; i < M;i++)	{
				printf("creating taskOne Thread %ld\n", i);
				// will be deallocated inside worker
				ThreadArgsMain* pArgs = (ThreadArgsMain *)malloc(sizeof(ThreadArgsMain));

			        mpz_init(pArgs->base_publickey.x);
			        mpz_init(pArgs->base_publickey.y);
			       	mpz_set(pArgs->base_publickey.x, base_publickey.x);
			       	mpz_set(pArgs->base_publickey.y, base_publickey.y);
			        
				mpz_init(pArgs->sum_publickey.x);
			        mpz_init(pArgs->sum_publickey.y);
				mpz_set(pArgs->sum_publickey.x, sum_publickey.x);
				mpz_set(pArgs->sum_publickey.y, sum_publickey.y);
				
				mpz_init(pArgs->negated_publickey.x);
			        mpz_init(pArgs->negated_publickey.y);
				mpz_set(pArgs->negated_publickey.x, negated_publickey.x);
				mpz_set(pArgs->negated_publickey.y, negated_publickey.y);
				
				mpz_init(pArgs->dst_publickey.x);
			        mpz_init(pArgs->dst_publickey.y);
				mpz_set(pArgs->dst_publickey.x, dst_publickey.x);
				mpz_set(pArgs->dst_publickey.y, dst_publickey.y);

				mpz_init(pArgs->target_publickey.x);
			        mpz_init(pArgs->target_publickey.y);
				mpz_set(pArgs->target_publickey.x, target_publickey.x);
				mpz_set(pArgs->target_publickey.y, target_publickey.y);
				
				pArgs->pMutex = &mutex;
				pArgs->pSemaphore = &semaphore;
				pArgs->pOutput = OUTPUT;
				
				mpz_init(pArgs->base_key);
				mpz_set(pArgs->base_key, base_key);

				mpz_init(pArgs->sum_key);
				mpz_set(pArgs->sum_key, sum_key);
				QueueNode* pNode = createWorkNode(taskOne, pArgs);
				
				
				pushNode(pNode, &(pThreadPool->pHead), &(pThreadPool->pTail), &(pThreadPool->queueMutex), &(pThreadPool->workCond));
			}
			
			sem_wait(&semaphore);
	                printf("FLAG_FORMAT %d\n", FLAG_FORMART);
			switch(FLAG_FORMART)	{
				case 0: //Publickey
					generate_strpublickey(&target_publickey,FLAG_LOOK == 0,str_publickey);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_publickey);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_publickey);
					}
				break;
				case 1: //rmd160
					generate_strrmd160(&target_publickey,FLAG_LOOK == 0,str_rmd160);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_rmd160);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_rmd160);
					}
				break;
				case 2:	//address
					generate_straddress(&target_publickey,FLAG_LOOK == 0,str_address);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_address);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_address);
					}
				break;
			}
		}
		else	{
			sem_init(&semaphore, 0, 2-M);
			
			mpz_cdiv_q_ui(base_key,diff,M);
			Scalar_Multiplication(G,&base_publickey,base_key);
			mpz_set(sum_publickey.x,base_publickey.x);
			mpz_set(sum_publickey.y,base_publickey.y);
			mpz_set(sum_key,base_key);
			for(i = 0; i < M;i++)	{
				//printf("creating taskTwo Thread %ld\n", i);
				// will be deallocated inside worker
				ThreadArgsMain* pArgs = (ThreadArgsMain *)malloc(sizeof(ThreadArgsMain));
				
			        mpz_init(pArgs->base_publickey.x);
			        mpz_init(pArgs->base_publickey.y);
			       	mpz_set(pArgs->base_publickey.x, base_publickey.x);
			       	mpz_set(pArgs->base_publickey.y, base_publickey.y);
			        
				mpz_init(pArgs->sum_publickey.x);
			        mpz_init(pArgs->sum_publickey.y);
				mpz_set(pArgs->sum_publickey.x, sum_publickey.x);
				mpz_set(pArgs->sum_publickey.y, sum_publickey.y);
				
				mpz_init(pArgs->negated_publickey.x);
			        mpz_init(pArgs->negated_publickey.y);
				mpz_set(pArgs->negated_publickey.x, negated_publickey.x);
				mpz_set(pArgs->negated_publickey.y, negated_publickey.y);
				
				mpz_init(pArgs->dst_publickey.x);
			        mpz_init(pArgs->dst_publickey.y);
				mpz_set(pArgs->dst_publickey.x, dst_publickey.x);
				mpz_set(pArgs->dst_publickey.y, dst_publickey.y);

				mpz_init(pArgs->target_publickey.x);
			        mpz_init(pArgs->target_publickey.y);
				mpz_set(pArgs->target_publickey.x, target_publickey.x);
				mpz_set(pArgs->target_publickey.y, target_publickey.y);
				
				pArgs->pMutex = &mutex;
				pArgs->pSemaphore = &semaphore;
				pArgs->pOutput = OUTPUT;
				
				mpz_init(pArgs->base_key);
				mpz_set(pArgs->base_key, base_key);

				mpz_init(pArgs->sum_key);
				mpz_set(pArgs->sum_key, sum_key);

				QueueNode* pNode = createWorkNode(taskTwo, pArgs);
				pushNode(pNode, &(pThreadPool->pHead), &(pThreadPool->pTail), &(pThreadPool->queueMutex), &(pThreadPool->workCond));
      				
				Point_Addition(&(pArgs->sum_publickey), &(pArgs->base_publickey), &(pArgs->dst_publickey));
      				mpz_set(pArgs->sum_publickey.x, pArgs->dst_publickey.x);
      				mpz_set(pArgs->sum_publickey.y, pArgs->dst_publickey.y);
      				mpz_add(sum_key,sum_key,base_key);
			}
			
			sem_wait(&semaphore);
	                //printf("semaphore signaled, FLAG_FORMAT %d, value of M %ld\n", FLAG_FORMART, M);
			switch(FLAG_FORMART)	{
				case 0: //Publickey
					generate_strpublickey(&target_publickey,FLAG_LOOK == 0,str_publickey);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_publickey);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_publickey);
					}
				break;
				case 1: //rmd160
					generate_strrmd160(&target_publickey,FLAG_LOOK == 0,str_rmd160);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_rmd160);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_rmd160);
					}
				break;
				case 2:	//address
					generate_straddress(&target_publickey,FLAG_LOOK == 0,str_address);
					if(FLAG_HIDECOMMENT)	{
						fprintf(OUTPUT,"%s\n",str_address);
					}
					else	{
						fprintf(OUTPUT,"%s # target\n",str_address);
					}
				break;
			}
		}
		
		mpz_clear(base_publickey.x);
		mpz_clear(base_publickey.y);
		mpz_clear(sum_publickey.x);
		mpz_clear(sum_publickey.y);
		mpz_clear(negated_publickey.x);
		mpz_clear(negated_publickey.y);
		mpz_clear(dst_publickey.x);
		mpz_clear(dst_publickey.y);
		mpz_clear(base_key);
		mpz_clear(sum_key);
	}
	else	{
		fprintf(stderr,"Version: %s\n",version);
		fprintf(stderr,"[E] there are some missing parameter\n");
		showhelp();
	}

	//printf("Main task completed, destroying threadpool.\n");
	destroyThreadPool(pThreadPool);
	printf("Main task completed.\n");
	return 0;
}

void showhelp()	{
	printf("\nUsage:\n-h\t\tshow this help\n");
	printf("-b bits\t\tFor some puzzles you only need a bit range\n");
	printf("-f format\tOutput format <publickey, rmd160, address>. Default: publickey\n");
	printf("-l look\t\tOutput <compress, uncompress>. Default: compress\n");
	printf("-n number\tNumber of publikeys to be geneted, this numbe will be even\n");
	printf("-o file\t\tOutput file, if you omit this option the out will go to the standar output\n");
	printf("-p key\t\tPublickey to be substracted compress or uncompress\n");
	printf("-r A:B\t\trange A to B\n");
	printf("-R\t\tSet the publickey substraction Random instead of secuential\n");
	printf("-x\t\tExclude comment\n\n");
	printf("Developed by albertobsd\n\n");
}

void set_bit(char *param)	{
	mpz_t MPZAUX;
	int bitrange = strtol(param,NULL,10);
	if(bitrange > 0 && bitrange <=256 )	{
		mpz_init(MPZAUX);
		mpz_pow_ui(MPZAUX,TWO,bitrange-1);
		mpz_set(min_range,MPZAUX);
		mpz_pow_ui(MPZAUX,TWO,bitrange);
		mpz_sub_ui(MPZAUX,MPZAUX,1);
		mpz_set(max_range,MPZAUX);
		gmp_fprintf(stderr,"[+] Min range: %Zx\n",min_range);
		gmp_fprintf(stderr,"[+] Max range: %Zx\n",max_range);
		mpz_clear(MPZAUX);
	}
	else	{
		fprintf(stderr,"[E] invalid bit param: %s\n",param);
		exit(0);
	}
}

void set_publickey(char *param)	{
	char hexvalue[65];
	char *dest;
	int len;
	len = strlen(param);
	dest = (char*) calloc(len+1,1);
	if(dest == NULL)	{
		fprintf(stderr,"[E] Error calloc\n");
		exit(0);
	}
	memset(hexvalue,0,65);
	memcpy(dest,param,len);
	trim(dest," \t\n\r");
	len = strlen(dest);
	switch(len)	{
		case 66:
			mpz_set_str(target_publickey.x,dest+2,16);
		break;
		case 130:
			memcpy(hexvalue,dest+2,64);
			mpz_set_str(target_publickey.x,hexvalue,16);
			memcpy(hexvalue,dest+66,64);
			mpz_set_str(target_publickey.y,hexvalue,16);
		break;
	}
	if(mpz_cmp_ui(target_publickey.y,0) == 0)	{
		mpz_t mpz_aux,mpz_aux2,Ysquared;
		mpz_init(mpz_aux);
		mpz_init(mpz_aux2);
		mpz_init(Ysquared);
		mpz_pow_ui(mpz_aux,target_publickey.x,3);
		mpz_add_ui(mpz_aux2,mpz_aux,7);
		mpz_mod(Ysquared,mpz_aux2,EC.p);
		mpz_add_ui(mpz_aux,EC.p,1);
		mpz_fdiv_q_ui(mpz_aux2,mpz_aux,4);
		mpz_powm(target_publickey.y,Ysquared,mpz_aux2,EC.p);
		mpz_sub(mpz_aux, EC.p,target_publickey.y);
		switch(dest[1])	{
			case '2':
				if(mpz_tstbit(target_publickey.y, 0) == 1)	{
					mpz_set(target_publickey.y,mpz_aux);
				}
			break;
			case '3':
				if(mpz_tstbit(target_publickey.y, 0) == 0)	{
					mpz_set(target_publickey.y,mpz_aux);
				}
			break;
			default:
				fprintf(stderr,"[E] Some invalid bit in the publickey: %s\n",dest);
				exit(0);
			break;
		}
		mpz_clear(mpz_aux);
		mpz_clear(mpz_aux2);
		mpz_clear(Ysquared);
	}
	//printf("freeing dest %p in %s\n", dest, __FUNCTION__);
	free(dest);
}

void set_range(char *param)	{
	Tokenizer tk;
	char *dest;
	int len;
	len = strlen(param);
	dest = (char*) calloc(len+1,1);
	if(dest == NULL)	{
		fprintf(stderr,"[E] Error calloc\n");
		exit(0);
	}
	memcpy(dest,param,len);
	dest[len] = '\0';
	stringtokenizer(dest,&tk);
	if(tk.n == 2)	{
		mpz_init_set_str(min_range,nextToken(&tk),16);
		mpz_init_set_str(max_range,nextToken(&tk),16);
	}
	else	{
		fprintf(stderr,"%i\n",tk.n);
		fprintf(stderr,"[E] Invalid range expected format A:B\n");
		exit(0);
	}
	freetokenizer(&tk);
	printf("freeing dest %p in %s\n", dest, __FUNCTION__);
	free(dest);
}

void set_format(char *param)	{
	int index = indexOf(param,formats,3);
	if(index == -1)	{
		fprintf(stderr,"[E] Unknow format: %s\n",param);
	}
	else	{
		FLAG_FORMART = index;
	}
}

void set_look(char *param)	{
	int index = indexOf(param,looks,2);
	if(index == -1)	{
		fprintf(stderr,"[E] Unknow look: %s\n",param);
	}
	else	{
		FLAG_LOOK = index;
	}
}


void generate_strpublickey(struct Point *publickey,bool compress,char *dst)	{
	memset(dst,0,132);
	if(compress)	{
		if(mpz_tstbit(publickey->y, 0) == 0)	{	// Even
			gmp_snprintf (dst,67,"02%0.64Zx",publickey->x);
		}
		else	{
			gmp_snprintf(dst,67,"03%0.64Zx",publickey->x);
		}
	}
	else	{
		gmp_snprintf(dst,131,"04%0.64Zx%0.64Zx",publickey->x,publickey->y);
	}
}

void generate_strrmd160(struct Point *publickey,bool compress,char *dst)	{
	char str_publickey[131];
	char bin_publickey[65];
	char bin_sha256[32];
	char bin_rmd160[20];
	memset(dst,0,42);
	if(compress)	{
		if(mpz_tstbit(publickey->y, 0) == 0)	{	// Even
			gmp_snprintf (str_publickey,67,"02%0.64Zx",publickey->x);
		}
		else	{
			gmp_snprintf(str_publickey,67,"03%0.64Zx",publickey->x);
		}
		hexs2bin(str_publickey,bin_publickey);
		sha256(bin_publickey, 33, bin_sha256);
	}
	else	{
		gmp_snprintf(str_publickey,131,"04%0.64Zx%0.64Zx",publickey->x,publickey->y);
		hexs2bin(str_publickey,bin_publickey);
		sha256(bin_publickey, 65, bin_sha256);
	}
	RMD160Data((const unsigned char*)bin_sha256,32, bin_rmd160);
	tohex_dst(bin_rmd160,20,dst);
}

void generate_straddress(struct Point *publickey,bool compress,char *dst)	{
	char str_publickey[131];
	char bin_publickey[65];
	char bin_sha256[32];
	char bin_digest[60];
	size_t pubaddress_size = 42;
	memset(dst,0,42);
	if(compress)	{
		if(mpz_tstbit(publickey->y, 0) == 0)	{	// Even
			gmp_snprintf (str_publickey,67,"02%0.64Zx",publickey->x);
		}
		else	{
			gmp_snprintf(str_publickey,67,"03%0.64Zx",publickey->x);
		}
		hexs2bin(str_publickey,bin_publickey);
		sha256(bin_publickey, 33, bin_sha256);
	}
	else	{
		gmp_snprintf(str_publickey,131,"04%0.64Zx%0.64Zx",publickey->x,publickey->y);
		hexs2bin(str_publickey,bin_publickey);
		sha256(bin_publickey, 65, bin_sha256);
	}
	RMD160Data((const unsigned char*)bin_sha256,32, bin_digest+1);
	
	/* Firts byte 0, this is for the Address begining with 1.... */
	
	bin_digest[0] = 0;
	
	/* Double sha256 checksum */	
	sha256(bin_digest, 21, bin_digest+21);
	sha256(bin_digest+21, 32, bin_digest+21);
	
	/* Get the address */
	if(!b58enc(dst,&pubaddress_size,bin_digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
}
