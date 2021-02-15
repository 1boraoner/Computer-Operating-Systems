/*
    Student Name: Bora Öner         Date: 3.06.2020
    Student Id: 150170301
    to compile: gcc -Wall -Werror -pthread main.c -o main
    to run: ./main #num1 #inc_process #dec_process #inc_turn #dec_turn

*/

#include <unistd.h> //fork(), exit()
#include <stdio.h> //printf()
#include <stdlib.h> //atoi(), malloc(), exit(), free
#include <sys/shm.h>  //shmget(), shmat(), shmctl(), shmdt()
#include <sys/types.h> //key_t
#include <sys/ipc.h> // IPC_RMID, IPC_CREAT 
#include <semaphore.h> //sem_t, sem_post(), sem_wait(), sem_init(), sem_destroy()
#include <stdbool.h> //bool data type
#include <signal.h> //kill(), pause(), struct sigaction,

/**********************SIGNAL FUNCTİONS******************/
void mysignal(int signum)
{
    //no action just signal
}

void mysigset(int num)
{
    struct sigaction mysigaction;
    mysigaction.sa_handler = (void*) mysignal;

    mysigaction.sa_flags = 0;

    sigaction(num, &mysigaction, NULL);
}

enum state {WAIT, CHANGE}; //states that processes are in some certian conditions
enum loops{LOOP,FINISH};   //used to identify the end of turn 

sem_t* incsem;             //semaphores used between increasing process for safety in their critical section
sem_t* decsem;             //semaphore used between decreasing process for safety in critical sectionss
sem_t* mutex;              //for decreasing turns
sem_t* tumex;              //for increasing turns
sem_t** increaser_semarr;  //semapahores for increasing prcess
sem_t** decreaser_semarr;  //seamphores for decreaser process

int* increaser_states;     //states of incereaseing process
int* decreaser_states;     //states of decreasing process

int* increaser_loop_states;//states of the each process's turn is finished or not
int* decreaser_loop_states;//states of the each process's turn is finished or not

int* coinbox;              //main money 
int* fibon;                //holds the array of fibonacci number sequence of each process

int* loop_cond;            //flag that makes the increasing loops make a certain money before decreaser process start
int* end_cond;             //flag that checks if the money is less than the fibonacci number 

int* pids;                 //holds every process pid includeing master process at zeroth index

void incr(int, int,int,int,int); //increasing function
void activation(int,bool);       //changes states of the proces according to the bool flag
int test(int);                   //test whether all process are in WAİT condition

void decr(int,int,int);          //decreasing function
void dec_activation(int,bool);   //changes states of the process according to the bool flag
int dectest(int);                //tests decreasing process conditions
int fibonacci_gen(int n);        //returns the nth number in the fibonacci sequence

int main(int argc, char* argv[]){

    mysigset(12); //signal defined with number = 12 //the signal that decreaser process signals to master process
    mysigset(13); //signal defined with number = 13 //the signal that master process singlas to each child proces


    if(argc != 6){
        printf("Not enough arguments");
    }

    int limit = atoi(argv[1]);      //money limit
    int nincreaser = atoi(argv[2]); //number of increaser process
    int ndecreaser = atoi(argv[3]); //number of decreaser process
    int ti = atoi(argv[4]);         //amount of turn for incraser prcoess
    int td = atoi(argv[5]);         //amount of turn for decreaser process

    int* pids = malloc((nincreaser + ndecreaser + 1 )* sizeof(int));

/********************************************ALL SEMAPHORES*****************************/

    key_t keysem  = ftok("main.c", 100);//key for mysem semaphore
    key_t keysem1 = ftok("main.c",101); //key for ipcsem semaphore
    key_t keysem6 = ftok("main.c",102); //key for mutex semaphore
    key_t keysem7 = ftok("main.c",110); //key for mutex op 2
    key_t keysem2 = ftok("main.c",111); //key for increaser semphore array
    key_t keysem5 = ftok("main.c",112); //key for decresaer semphore array
    key_t keysem3[nincreaser];          //increaer process key
    key_t keysem4[ndecreaser];          //decreaser process key

    int shmsemid  = shmget(keysem,sizeof(sem_t),0666|IPC_CREAT);                  //shared memory segment id for incsem counting semaphore
    int decsemid = shmget(keysem1,sizeof(sem_t),0666|IPC_CREAT);                  //shared memory segment id for decsem counting semaphore
    int mutexshmid = shmget(keysem6,sizeof(sem_t),0666|IPC_CREAT);                //shared memory segment id for mutex binary semaphore
    int tumexshmid = shmget(keysem7,sizeof(sem_t),0666|IPC_CREAT);                //shared memory segment id for tumex binary semaphore
    int isem_shmid= shmget(keysem2, nincreaser * sizeof(sem_t*), 0666|IPC_CREAT); //increaser semaphores array pointer
    int shmsemid5 = shmget(keysem5, ndecreaser * sizeof(sem_t*), 0666|IPC_CREAT); //decreaser semaphores array pointer
    int inc_sem_arr[nincreaser];                                                  //each process has their own semaphore shared memory id
    int dec_sem_arr[ndecreaser];                                                  //each process has their own semaphore shared memory id
    
    
    
    incsem  = (sem_t*) shmat(shmsemid,(void*)0,0);            //sem for synchornization among inc process
    decsem = (sem_t*) shmat(decsemid,(void*)0,0);             //sem for synchornication among dec pross
    mutex = (sem_t*) shmat(mutexshmid,(void*)0,0);            //comminication of inc and dec
    tumex = (sem_t*) shmat(tumexshmid,(void*)0,0);            //second ipcs mutex
    increaser_semarr = (sem_t**) shmat(isem_shmid,(void*)0,0); //sem for inc process arr
    decreaser_semarr = (sem_t**) shmat(shmsemid5,(void*)0,0); //sem for dec process arr


    int err = sem_init(incsem,1,1);             //increasing loop mutex for ciritical section
    int err1 = sem_init(decsem,1,1);            //decreasing loop mutex for critical section
    int err10 = sem_init(mutex,1,0);            //decresaer process semaphore
    int err12 = sem_init(tumex,1,0);            //increaser process semaphore

    if(err == -1){printf("Semaphore cannot be initialized\n");}
    if(err10 == -1){printf("Semaphore cannot be initialized\n");}
    if(err12 == -1){printf("Semaphore cannot be initialized\n");}
    if(err1 == -1){printf("Interprocess semaphore cannot be initialized\n");}

    int i;
    for(i=0;i<nincreaser;i++){
        
        keysem3[i] = ftok("main.c",103+i); //for each semaphore different key is generated
        
        inc_sem_arr[i] = shmget(keysem3[i],sizeof(sem_t),0666|IPC_CREAT);  //each key has shared memory
        
        increaser_semarr[i] = shmat(inc_sem_arr[i],(void*)0, 0); //for each process they have their unique semaphore
        
        int err2 = sem_init(increaser_semarr[i],1,0); // EACH PROCESS BINARY SEMAPHORE_LOCKS 
        if(err2 == -1){printf("%dth increaer semaphore cannot be initailzed\n",i);}
    }
    
    for(i=0;i<ndecreaser;i++){
        
        keysem4[i] = ftok("main.c",203+i);
        
        dec_sem_arr[i] = shmget(keysem4[i],sizeof(sem_t),0666|IPC_CREAT);
        
        decreaser_semarr[i] = shmat(dec_sem_arr[i],(void*)0, 0);
        
        int err2 = sem_init(decreaser_semarr[i],1,0); // EACH PROCESS BINARY SEMAPHORE_LOCKS 
        if(err2 == -1){printf("%dth decreaser semaphore cannot be initailzed\n",i);}
    }


/********************************************ALL SHARED VARIABLES*****************************/


    key_t shmkey  = ftok("main.c",1); //coinbox
    key_t shmkey1 = ftok("main.c",2); //fibonacci array 
    key_t shmkey2 = ftok("main.c",3); //increaser states {WAIT,CHANGE}
    key_t shmkey3 = ftok("main.c",4); //decreaser states {WAIT,CHANGE}
    key_t shmkey4 = ftok("main.c",5); //increaser loop conditions
    key_t shmkey5 = ftok("main.c",6); //decresaer loop conditions
    key_t shmkey6 = ftok("main.c",7); //loop cond için
    key_t shmkey7 = ftok("main.c",8); //end cond için
    
    int shmid = shmget(shmkey, sizeof(int),0666|IPC_CREAT);               //money in the box shmid
    int shmid1 = shmget(shmkey1, ndecreaser* sizeof(int),0666|IPC_CREAT); //fibonacci conatiner shmid
    int shmid2 = shmget(shmkey2, nincreaser* sizeof(int),0666|IPC_CREAT); //
    int shmid3 = shmget(shmkey3, ndecreaser* sizeof(int),0666|IPC_CREAT); //
    int shmid4 = shmget(shmkey4, nincreaser* sizeof(int),0666|IPC_CREAT); //
    int shmid5 = shmget(shmkey5, ndecreaser* sizeof(int),0666|IPC_CREAT); //
    int shmid6 = shmget(shmkey6, sizeof(int),0666|IPC_CREAT);
    int shmid7 = shmget(shmkey7, sizeof(int),0666|IPC_CREAT);


    end_cond = shmat(shmid7,(void*)0,0);
    *end_cond = 0;

    loop_cond = shmat(shmid6,(void*)0,0);
    *loop_cond = 0;

    coinbox = shmat(shmid,(void*)0,0);
    *coinbox=0; //initially the total money is zero

    fibon = shmat(shmid1,(void*)0,0);
    for(i =0;i<ndecreaser;i++){fibon[i] = 0;} //each decreaser process's initial fibonacci series index is 0

    increaser_states = shmat(shmid2, (void*)0,0);

    increaser_loop_states = shmat(shmid4, (void*)0,0);

    for(i=0;i<nincreaser;i++){
        increaser_states[i] = CHANGE;
        increaser_loop_states[i] = LOOP;

    }
    
    decreaser_states = shmat(shmid3, (void*)0,0);
    decreaser_loop_states = shmat(shmid5, (void*)0,0);

    for(i=0;i<ndecreaser;i++){
       decreaser_states[i] = CHANGE;
       decreaser_loop_states[i] = LOOP;

    }

    printf("Master Process: Current Money is 0 \n");

    //**********************************ALL SEMAPHORES AND SHARED VARİABLES ARE DONE*******************************//



    bool flag = false; //identifier among the increaser and decreaser processes
    int f;//holds the childer process pid

    int incpros_amount; //increasing amount of the inc prosses

    int thprs_inc =0;//increasing process index
    int thprs_dec =0; //decreasing process index

    pids[0] = getpid();
    for(i=0; i< nincreaser + ndecreaser;i++){

        f = fork();
        if(f==0){
            
            //pids[i+1] = getpid();

            if(i<nincreaser){

                flag = false;

                thprs_inc = i;

                if(i%2 == 0){
                    incpros_amount = 10; //process #even are always increase by 10
                }else{
                    incpros_amount = 15; //process #odd  are always increase by 15
                }

                break;

            }else{

                flag = true;
                thprs_dec = i- nincreaser; //the index for decresing 
                break;
            }
            
        }else if(f == -1){
            printf("cannot fork so exit");
            exit(1);
        }else{
            pids[i+1] = f; //each child's pid is soted in he pids[] aray of the master process !!
        }
    }


    if(f != 0){ //MASTER PROCESS 

        
        pause(); //waiting for the kill signal from decreaseing precess
        
        for(i=1;i<ndecreaser + nincreaser+1;i++){
        
            int errff = kill(pids[i],13); //signaling all children

            if(errff == -1){
            printf("proses %d kill başarısız oldu \n",i);
            }
        
        }
        
        printf("Master is Terminating all child\n");

        free(pids); //dynamically allocated pids array is freed

/*********All semaphores are destroyed************/
        sem_destroy(incsem);
        sem_destroy(decsem);
        sem_destroy(mutex);
        sem_destroy(tumex);
        int u;
        for(u=0;u<nincreaser;u++){
            sem_destroy(increaser_semarr[u]);
        }
        for(u=0;u<ndecreaser;u++){
            sem_destroy(decreaser_semarr[u]);
        }

/*********All Shared Memory are detached************/

        shmdt(end_cond);
        shmdt(loop_cond);
        shmdt(coinbox);
        shmdt(fibon);

        shmdt(incsem);
        shmdt(decsem);
        shmdt(mutex);
        shmdt(tumex);

        shmdt(increaser_states);
        shmdt(decreaser_states);
        shmdt(increaser_loop_states);
        shmdt(decreaser_loop_states);
        int j;
        for(j=0;j<ndecreaser;j++)
            shmdt(decreaser_semarr[j]);
        
        shmdt(decreaser_semarr);
        
        for(j=0;j<nincreaser;j++)
            shmdt(increaser_semarr[j]);
        
        shmdt(increaser_semarr);

/*********All Shared Memory are deeleted************/

        shmctl(shmid,IPC_RMID,NULL);
        shmctl(shmid1,IPC_RMID,NULL);
        shmctl(shmid2,IPC_RMID, NULL);
        shmctl(shmid3,IPC_RMID, NULL);
        shmctl(shmid4,IPC_RMID, NULL);
        shmctl(shmid5,IPC_RMID, NULL);
        shmctl(shmid6,IPC_RMID,NULL);
        shmctl(shmid7,IPC_RMID,NULL);
    	shmctl(shmsemid,IPC_RMID,NULL);
		shmctl(decsemid,IPC_RMID,NULL);
        shmctl(isem_shmid,IPC_RMID,NULL);
        shmctl(shmsemid5,IPC_RMID,NULL);
        shmctl(mutexshmid,IPC_RMID,NULL);
        shmctl(tumexshmid,IPC_RMID,NULL);
        int k;
		for(k=0;k<nincreaser;k++){
    		shmctl(inc_sem_arr[k],IPC_RMID,NULL);
        }
        for(k=0;k<ndecreaser;k++){
    		shmctl(dec_sem_arr[k],IPC_RMID,NULL);
        }

    }else{
        

        int turn =0; //turn is used in for keep track of the turn of the processes. 
        while(*coinbox > 0 ||turn == 0){

            if(flag == false){      //increasing process go inside
                
            
                for(i=0;i<ti;i++){ //each process goes runs for ti turns and invokes incr() function 
                    incr(incpros_amount,thprs_inc,nincreaser,ti*turn + i,limit);
                }

                sem_wait(incsem); //
                
                    increaser_loop_states[thprs_inc] = FINISH; //each process that finish their ti turns states become FINISH

                    int k=0;
                    int j;
                    for(j=0;j<nincreaser;j++){ //the last process will count the states of process to detect if all process are finished
                        if(increaser_loop_states[j] == FINISH){
                            k++; 
                        }
                    }
                    if(k == nincreaser){ //the lsat process comes here. In this block the semaphore that holds decrementer process will be unlocked 
                        
                        int m;
                        for(m=0;m<nincreaser;m++){
                            increaser_loop_states[m] = LOOP; //switching the states
                        }
                        if(*loop_cond == 1){  //after the necesseary money amount is met the decrementing and incrementing fucntions are go back to back
                            printf("********************************\n");

                            int t;
                            for(t=0;t<ndecreaser;t++){
                                sem_post(mutex); //openning the counting semaphores of decrementing process
                            }
                        }
                    }
                
                                       
                turn++;  //for keep track pf the turns of the incrementinn process               
                sem_post(incsem); //the ciritcal section ends
                if(*loop_cond == 1){
                    sem_wait(tumex); //incrementin process will lock it self and it will be unlocked after decrementing proces finish thier work
                }

            }else{             //decreasing process go inside the else block statement
                
                sem_wait(mutex); //decrementing process sempahore, it will lock itself after their turn is finished

                int d;
                for(d=0;d<td;d++){
                    if(*coinbox<= 0){break;}
                    decr(thprs_dec,ndecreaser,td*turn + d);
                }

                sem_wait(decsem);
                decreaser_loop_states[thprs_dec] = FINISH; //same algorithm with incrementing process

                int k=0;
                int j;
                for(j=0;j<ndecreaser;j++){
                    if(decreaser_loop_states[j] == FINISH){
                        k++;
                    }
                }
                if(k == ndecreaser){

                    printf("********************************\n");
                    
                    int u;
                    for(u=0;u<ndecreaser;u++){
                        decreaser_loop_states[u] = LOOP;
                    }

                    for(u=0;u<nincreaser;u++){
                       sem_post(tumex); //increments the semaphore that incremeninting process wait
                      }
                }           
                turn++;

                sem_post(decsem); //ciritical section ends
                if(*coinbox<= 0){break;}
            }
        }
            free(pids); //cilhdren do not use pids array so it is freed
    }


    return 0;
}


//*****************************************INCREASER FUNCTİONS************************************//

void incr(int amount,int pindex, int npr,int turn,int limit){

    sem_wait(incsem); //incsem has value 1 so it locks the incoming process after one is inside the critical section

    *coinbox += amount;  //total money is increased by the specific value 10 or 15 depending on the process index(pindex)

    increaser_states[pindex] = WAIT; //after inceremention its state becomes WAIT meaning it is finished its work

    int waitcond = test(npr); //test if all process are in WAIT condition?

    if(waitcond == 0){ //if not all process is in wait condition the process will lock itself after incrementing the value of incsem semaphore allowing waiting process to go ciritcal section
        printf("Increaser Process %d: Current Money is %d \n",pindex,*coinbox);

        sem_post(incsem); //open lock
        sem_wait(increaser_semarr[pindex]); //lock itself waiting to be unlocked 

    }else{ //if all process are in WAIT state the last process in this turn will unlock all other process to allow each process to continue in the loop 

        printf("Increaser Process %d: Current Money is %d, increaser process finished their turn %d \n",pindex,*coinbox,turn+1);

        if(*coinbox >= limit){*loop_cond = 1;} //loop_condition is for the initial increment process is if it is finished or not?
        
        activation(npr,true); //all states become CHANGE
        
        int i;
        for(i=0;i<npr;i++){
            sem_post(increaser_semarr[i]); //every semaphore is incremented so that each process conitnues their other turns
        }

        sem_wait(increaser_semarr[pindex]); //decresing mutex becasue it becomes 2 instead of 1
        
        sem_post(incsem); //end of CRITICAL SECTION
    }    
}

int test(int npross){ //checks if the all increaser process are in wait conddition

    int flag = 0;

    int i;
    for(i=0;i<npross;i++){

        if(increaser_states[i] == WAIT){
            flag += 1;
        }
    }
    if(flag == npross){
        return 1;
    }else{
        return 0;
    }


}

void activation(int npr,bool f){ //changes the states of the increaser processes
    if(f == 1){
        int i;
        for(i=0;i<npr;i++){
            increaser_states[i] = CHANGE; //makes each state to CHANGE
        }
    }else{
        int i;
        for(i=0;i<npr;i++){
            increaser_states[i] = WAIT; //makes each state to WAIT
        }
    }
}

//*****************************************DECREASER FUNCTİONS***********************************//

void decr(int pindex, int npr,int turn){

    
    sem_wait(decsem);

    if(*end_cond == 0){
        //bool end_cond = false;
        bool flag = false; //flag for determining the odd and even decreasers in the cases of money in the box is odd or even

        bool availibity = false; //flag for avalibilty in the decreaser if odd or even proces is left in cases

        if (((pindex % 2 == 0 && *coinbox % 2 != 0) || (pindex % 2 != 0 && *coinbox % 2 == 0))){ //makes sure if money is odd, odd process works or if even, even works
            flag = true;
        }else{

            int amount = fibonacci_gen(fibon[pindex]); //fibonacci number at the index

            if (amount >= *coinbox){ //decrementing
                *coinbox -= amount;
                *end_cond = 1; //end condition flag becomes 1 if the decreasing amount is hgigher than current money
            }else{
                *coinbox -= amount;
            }
        }

        decreaser_states[pindex] = WAIT; //decreaser process makes it self WAIT state

        int waitcond = dectest(npr); //check if all decreaser is in WAIT condition

        
        if(!flag){ //are there any even or odd decreaser process left? 
            int counter = 0;

            if(*coinbox % 2 == 0){ //if money is even look for even decreasers
                int i;
                for(i=0;i<npr;i++){

                    if(i%2==0){
                        if(decreaser_states[i] == CHANGE){
                            counter++;
                        }
                    }
                }
            }else{ //if money is odd look for odd decreasers

                int i;
                for(i=0;i<npr;i++){

                    if(i%2!=0){
                        if(decreaser_states[i] == CHANGE){
                            counter++;
                        }
                    }
                }
            }   

            if(counter!=0){
                availibity = true; //flag becames true if there is an available decreaser
            }

        }

        if (waitcond == 0 ){ //normal process

            if (*end_cond == 1){ //end condition
                int amount = fibonacci_gen(fibon[pindex]);
                printf("Decreaser Process %d: Current Money is less than %d, signaling master to finish(%dth fibonacci number for decreaser %d) \n", pindex, amount, fibon[pindex], pindex);
                sem_post(decsem); 

                int x = getppid();
                kill(x, 12); //signal master to kill all children


            }else{
                if (!flag){ 
                    
                    if(availibity){ //odd or even process kalmama durumu için
                        printf("Decreaser Process %d: Current Money is %d (%dth Fibonacci number for decreaser %d)\n", pindex, *coinbox, ++fibon[pindex], pindex);
                    }else{ //normal condition
                        printf("Decreaser Process %d: Current Money is %d (%dth Fibonacci number for decreaser %d), decreaser process finished their turn %d\n", pindex, *coinbox, ++fibon[pindex], pindex, turn + 1);
                    }
                }

                sem_post(decsem); //process allows waiting process to come into the ciritical section in the function
                sem_wait(decreaser_semarr[pindex]); //process locks itself
            }
        }else{ //son process ise giriyo

            if (*end_cond != 1){ //end condition
                
                if(!flag){
                    printf("Decreaser Process %d: Current Money is %d (%dth Fibonacci number for decreaser %d), decreaser process finished their turn %d\n", pindex, *coinbox, ++fibon[pindex], pindex, turn + 1);
                }
            }else{
                
                int amount = fibonacci_gen(fibon[pindex]); 

                printf("Decreaser Process %d: Current Money is less than %d, signaling master to finish(%dth fibonacci number for decreaser %d) \n", pindex, amount, fibon[pindex], pindex);
                int x = getppid();
                kill(x, 12); 
            }


            dec_activation(npr, true);

            int i;
            for (i = 0; i < npr; i++){
                sem_post(decreaser_semarr[i]); //the last decreaser process will unlock all mutexes of decreaser process 
            }

            sem_wait(decreaser_semarr[pindex]); //decresing mutex becasue it becomes 2 in the loop instead of 1
            
            sem_post(decsem); //end of ciritical section
        }
    }
}

int dectest(int npross){ //checks if all states of the decreasers process are in wait condition

    int flag = 0;
   
   int i;
    for(i=0;i<npross;i++){

        if(decreaser_states[i] == WAIT){
            flag += 1;
        }
    }
    if(flag == npross){
        return 1;
    }else{
        return 0;
    }
}

void dec_activation(int npr, bool f){ //changes the states of the decreaser processes
    
    if(f == true){
        int i;
        for(i=0;i<npr;i++){
            decreaser_states[i] = CHANGE; //makes each state to CHANGE
        }
    }else{
        int i;
        for(i=0;i<npr;i++){
            decreaser_states[i] = WAIT; //makes each state to WAIT
        }
    }
}

int fibonacci_gen(int n){ //returns the th index fibonacci number of fibonacci sequence

    if(n == 0 || n == 1){
        return 1;
    }else{

        int val = 0 ;
        int val_1 = 1;
        int val_2 = 1;

        int i;
        for(i=2;i<=n;i++){
            val  = val_1 + val_2;
            val_1 = val_2;
            val_2 = val;
        }
        return val;
    }
}
