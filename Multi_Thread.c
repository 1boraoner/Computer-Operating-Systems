//Student ID: 150170301 Student Name: Bora Öner 

//to compile: gcc -pthread -Wall -Werror 150170301.c -o main
//to run: ./main #lower #upper #process #threads exmample: ./main 101 200 2 2

#include <stdio.h>
#include <unistd.h>//fork()
#include <stdlib.h>
#include <pthread.h>//pthread_create(),
#include <string.h>//string 
#include <sys/ipc.h> //IPC_CREAT, IPC_RMID
#include <sys/shm.h> //shmget(), shmat(), shmdt(),shmctl()
#include <sys/wait.h>//wait()


typedef  struct node{//Binary Search TREE is defined it will be used to hold prime numbers as its data

        int data;
        struct node* left;
        struct node* right;

}node;

struct composite{//This structure will be used as thread starter fucntion as argument it contains every necessary variable
	struct node* root_container; //holds the address of root
	struct node* next_container; //holds the address of prime_container array
	int lower; //lower bound
	int upper; //upper bound
	int offset; //I divided the prime_container array by these offsets to that all threads have own working space in terms of adding prime numbers prime_container array.
	int diff;
};

void* thread_start(void* p1); //THREAD INITİLZER
void add_to_tree(struct node* root, struct node* nextrarr);//Add nodes to the BST
void prime_finder(struct node* root, struct node* prime_container, int lower_bound, int upper_bound,int offset,int diff);//finds the prime in the given interval
void print_tree(struct node* t);//prints the tree recursively


int main(int argc, char* argv[]){
	
	if(argc != 5){
		printf("NOT ENOUGH ARGUMENTS\n");
		exit(1);
	}

	int lower = atoi(argv[1]);
	int upper = atoi(argv[2]);
	int n_process = atoi(argv[3]);
	int n_thread = atoi(argv[4]);
			

	key_t KEY1 = ftok("150170301.c",1); //unique key is generated
	int shmid = shmget(KEY1,sizeof(struct node),0666|IPC_CREAT); //shared memory segment id is created using the unique key
	struct node* root = shmat(shmid,(void*)0,0); //root node is created and shared among all upcoming processes
	root->data = -100;//initial and invalid data is given as to distinguish for the first node to be added.
	root->left = NULL;
	root->right = NULL;
	
	key_t KEY2 = ftok("150170301.c",2); //unique key is generated
	int size = sizeof(struct node)*(upper-lower+10);//the size of dynamic array is defined
	int shmid_0 = shmget(KEY2,size,0666|IPC_CREAT);	//shared memory segment id is created and this segment is actually an dynmaic array
	struct node* prime_container  = shmat(shmid_0,(void*)0,0);//prime container variable is the dynamic array which holds the all prime numbers that are found and this array will be basis for my BST TREE since malloc() cannot be used
	

	int the_int = (upper - lower)/n_process; //the values of the intervals is found by the difference of interval dividing the number of process 

	printf("Master Started.\n");//PRİNT MASTER
	
	int interval[2]; //holds the interval values
	int f;
	int counter = 0;//counter for process number
	int i;
	for(i=0;i<n_process;i++){
			
			f = fork(); //generates child process
			if(f==0){
					interval[0] =lower;  
					interval[1] =lower + the_int; //the diff  
		
				if(i+1 == n_process){
					interval[1] = upper; //for the last process the upper bound is set directly to the users upper for precaution to get out of bound
				}
					break; //break for child process
			}
		
		counter++;
			
		lower = lower + the_int + 1;
	}
	

	
	if(f > 0){//MASTER PROCESS

		while(wait(NULL)>0);//waits all children to execute
		printf("Master: Done. Prime Numbers are:");
		print_tree(root);//prints all prime numbers
		printf("\n");
		shmdt(root);//detaching the shared memory segment from master
		shmdt(prime_container);//detaching hte shared memory segment from master
		shmctl(shmid,IPC_RMID,NULL);//the shared memory is released
		shmctl(shmid_0,IPC_RMID,NULL);//the shared memory is released
		
	}else{ //ALL CHİLD PROCESS 
			
			
		printf("Slave %d: Started. Interval [%d %d]\n",counter+1,interval[0],interval[1]);
			
		int sub_interval[2]; //the sub_interval is for all threads and will be updated as he threads are created
		int diff = the_int/n_thread;	
		if((upper-lower)%n_thread*n_process != 0){diff--;}//some special cases the difference in threads needs to be decreadsed by 1 to divide the interval evenly			
		sub_interval[0] = interval[0];
		sub_interval[1] = interval[0] + diff;
	
			
		pthread_t threads[n_thread];//thread array is created
		int j;
		for(j=0; j < n_thread; j++){
				
				
			sub_interval[1] = sub_interval[0] + diff;
			if((j+1 == n_thread)){
				sub_interval[1] = interval[1];
				if(counter +1 == n_process){
					sub_interval[1] = upper;
				}
			}
			int off_set = counter*n_thread + j;//I think threads as 2D array so I used the (i*COLOUMN + j) transformation formula to think like 1D array of threads this will be the index numbers of threads
			struct composite A = {root,prime_container,sub_interval[0],sub_interval[1], off_set, diff};//composite struct is defined in each thread
			struct composite* composite_ptr = &A;//a ptr is defined for void* argument of thread runners
								
				
			printf("Thread %d.%d: searching in [%d %d]\n",counter+1,j+1,sub_interval[0],sub_interval[1]);
				
				
			int error = pthread_create (&threads[j], NULL, (void* ) &thread_start, (void* )composite_ptr);//creates the threads and invokes thred_start funciton
				
			if (error != 0) {printf("\nThread can't be created :[%s]",strerror(error)); }//if thread is not initialized this error message is printed


			pthread_join(threads[j], NULL);//each thread is joined
			sub_interval[0] = sub_interval[1] + 1;
		}
		printf("Slave %d: Done.\n",counter+1);
		shmdt(root);//After each child is completed its work the shared memory segment is detached form it so memory is safer
		shmdt(prime_container);
		exit(0); //exit
	}
	
	
	return 0;
}

void print_tree(struct node* t){//Printfs the bianry search tree recursively

    if(t){
        
        print_tree(t->left);
        printf("%d, ",t->data);
        print_tree(t->right);
        
    }

}
void add_to_tree(struct node* root, struct node* nextarr){//add the prime number to tree The working mechanism as follows; nextarr pointer is pointing a place shared memory in the prime_container array hat was shared and then the node is addaed to tree like malloc()
		
		
    if(root->left == NULL && root->right == NULL && root->data == -100){//to distinguish whether the root is NULL or not
        root->data = nextarr->data;
		
		return;
    }else{  //if root is not NULL       
			
		struct node* nn = nextarr; 
        nn->left= NULL;
        nn->right= NULL;
			
        struct node* t = root;
	
		int flag = 1;
        while(flag){ //Standart bianry search adding algorithm 
         
            if( nn->data < t->data ){
                    
                if(t->left == NULL){
                    t->left = nn;
                    flag=0;
					break;
                }else{                    
                    t = t->left;     
                }
            }
            else{
                if(t->right == NULL){  
					t->right = nn;
					flag = 0;
                    break;
                }else{
                    t = t->right;
                }
            }
			if(flag == 0){
				break;
			}
        }
    }	
}

void* thread_start(void* args){ //Threads are given the interval and in the function prime finder funciton is invoked
	

	struct composite* A_copy =  args;
	struct node* root_copy = A_copy->root_container;
	struct node* next_copy = A_copy->next_container;
	int lower_copy =A_copy->lower;
	int upper_copy = A_copy->upper;
	int offset_copy =  A_copy->offset;
	int diff_copy 	= 	A_copy->diff;
	
	prime_finder(root_copy,next_copy,lower_copy,upper_copy,offset_copy,diff_copy);
	
	pthread_exit(0);
}

void prime_finder(struct node* root, struct node* prime_container, int lower_bound, int upper_bound,int offset,int diff){//checks the numbers in the interval whether the number is prime or not and then calls the add function
	
	struct node* p = prime_container;//the pointer is assigned the shared array prime_container
	p = p + offset*diff;//the offset + diff is added to the p pointer to point the exact position of the prime_container array.
	int i;
	for(i = lower_bound;i<=upper_bound;i++){ //prime finding algoirthm
		if(i>1){
			int j;
			int isprime = 1;
			for(j=2; j<i; ++j){
				
				if(i%j==0){
					isprime = 0;
					break;
				}
			}
			if(isprime == 1){
				p->data = i; //the found prime number is assigned to the memory location that p points 
				add_to_tree(root,p);
				p++;//if the prime is found the pointer will be incremented by 1 so that 
			}
		}
	}
}
