#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include <pthread.h>
#include <emmintrin.h>
#include <x86intrin.h>


#define KB 1024
#define MB 1024 * 1024
#define CACHE_LINE_ELEMENTS 64/4
#define ITERATIONS 100

int curr_size;

void swap(int *a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

/* One element of this struct occupies an entire cache_line */
 
typedef struct {
	u_int32_t array[CACHE_LINE_ELEMENTS];
} cache_line;

cache_line *cache;

/* to_do : do we need to fill the cache randomly? */

void initialize_cache(int index_size) {
	
	int *trick1;
	int *trick2;

	/* With the help of the following pointers, we are tricking the prefetcher by not accessing 
	cache_lines that are next to each other */
	
	trick1 = (u_int32_t*)malloc(index_size*sizeof(u_int32_t));
	trick2 = (u_int32_t*)malloc(index_size*sizeof(u_int32_t));
		
	for ( int i = 0 ; i < index_size; i++){
		trick1[i]=i;
		trick2[i]=i;
	}
	
	/* Randomization to trick the prefetcher */
	
	for(int i = index_size-1; i >= 0; i--) {
		swap(&trick1[i], &trick1[rand() % (i+1)]);
		swap(&trick2[i], &trick2[rand() % (i+1)]);
	}
	
	
	/* filling up the cache */
	cache = (cache_line*)malloc(index_size*sizeof(cache_line));
	
	for(int i = 0; i < index_size; i++) {
		for (int f = 0; f < CACHE_LINE_ELEMENTS; f++) {
			cache[trick1[i]].array[f] = trick2[i];
		}
	}	
}



void *cache_time(void *vargp) {
	    
	clock_t start=0, end=0;
	double time_taken = 0, total_time_taken = 0;
	int index_size;
	int junk =0;
	volatile u_int32_t *addr;
	
	int sizes[] = {1*KB, 2*KB, 4*KB, 8*KB, 16*KB, 32*KB, 64*KB, 128*KB, 256*KB, 512*KB, 1*MB, 4*MB, 6*MB, 8*MB, 16*MB, 32*MB};
	
	for(int curr_size = 0; curr_size < sizeof(sizes)/sizeof(sizes[0]); curr_size++) {
		index_size = sizes[curr_size]/64;
//		initialize_cache(index_size);
		cache = (cache_line*)malloc(index_size*sizeof(cache_line));
		
	
		/* trying to randomize the access to cache as much as possible */
		
		int *trick1;
		int *trick2;
	
		/* With the help of the following pointers, we are tricking the prefetcher by not accessing 
		cache_lines that are next to each other */
		
		trick1 = (u_int32_t*) malloc(index_size*sizeof(u_int32_t));
		trick2 = (u_int32_t*) malloc(index_size*sizeof(u_int32_t));
			
		for ( int i = 0 ; i < index_size; i++){
			trick1[i]=i;
			trick2[i]=i;
		}
		
		/* Randomization to trick the prefetcher */
		
		for(int i = index_size-1; i >= 0; i--) {
			swap(&trick1[i], &trick1[rand() % (i+1)]);
			swap(&trick2[i], &trick2[rand() % (i+1)]);
		}
		
		/* populate the cache */
		for(int i = 0; i < index_size; i++) {
			addr = &cache[trick1[i]].array[0];
		}
		
		total_time_taken = 0;
		
		for(int iter=0; iter<ITERATIONS; iter++) {
			time_taken = 0;
			for(int i = 0; i < index_size; i++) {
				addr = &cache[trick1[i]].array[0];
				start = __rdtscp(&junk);
				junk = *addr ;
//				junk = junk & 0x7FFFFFFF;
//				*addr = junk;
				end = __rdtscp(&junk);
				time_taken += (end-start);
			}
			total_time_taken += (time_taken)/index_size;
		}
		printf("%d, %.8f \n", sizes[curr_size]/1024, total_time_taken/(ITERATIONS));
		
		
		/*flush out the entire cache array */
		
		for(int i = 0; i < index_size; i++) {
			for (int f = 0; f < CACHE_LINE_ELEMENTS; f++) {
				_mm_clflush(&cache[i].array[f]);
			}
		}
	}
}

int main(){
	
	int threads;
	
	pthread_t tid;
	
	printf("\nPlease enter an integer for number of threads: ");
	scanf("%d", &threads);
	
	for (int i=0; i<threads; i++)
		pthread_create(&tid, NULL, cache_time, (void *)&tid);
	
	pthread_exit(NULL);
	
	return 0;
}
	
