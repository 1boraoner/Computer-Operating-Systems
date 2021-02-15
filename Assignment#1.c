#include <unistd.h> // fork() getpid()
#include <stdlib.h>
#include <stdio.h>
#include<sys/wait.h> // wait()


int main(){

	int c1,c2,c3,c4; //it will store child ids in parents files
	int level =1; //level information
	int level2[3]={0,0,0};//used for parent file to store level 2 childs id
	
	for(int i=0; i<=2;i++){ 
	
		c1 = fork(); // level 2 child is created
		
		if(c1 == 0 ){ // level 2 child goes in
			level = 2;
			c2=fork(); // level 3 child is created
			
			if(c2 == 0 ){ // level 3 child goes in
				level = 3;
				printf("<%d,[ ],%d> \n",getpid(),level);
				break; // this child has no any other children so immediate break
				
			}else{
				wait(NULL); // level 2child waits its first child to be finished 
				level =2;
							
				c3 = fork(); // second child of level 2 is created in level 3
				if(c3==0){
					
					c4 = fork(); // level 3 childs children is created level 4
					if(c4==0){
						level = 4;
						printf("<%d,[ ],%d >\n",getpid(),level);
						break; // break is used to get out of loop not to iterate on the child again
					}else{
						wait(NULL); // level 3 child waits for its children to finish off 
						level = 3;
						printf("<%d,[ %d ],%d >\n",getpid(),c4,level);
						break; // break is used to get out of loop not to iterate on the child again
					}
				}else{
						wait(NULL);	
				}
			}
			level = 2;
			printf("<%d,[ %d %d ],%d> \n",getpid(),c2,c3,level);
			break;
			
		}else{
			
			wait(NULL);
			level2[i] = c1; // in each iteration c1 holds the ids for level 2 children they are stored in array
		
		}
		if(i==2){printf("<%d,[ %d %d %d ],%d> \n",getpid(),level2[0],level2[1],level2[2],level);}
	}
	return 0;
}
