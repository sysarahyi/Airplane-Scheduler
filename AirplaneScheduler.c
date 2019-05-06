#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdbool.h>

#define PLANES 100
#define TAKEOFF_TIME 3
#define LANDING_TIME 2

bool planeStates[PLANES];

sem_t pointerAccess;
sem_t runway;

char numberString[4];

int requestIndex;

void *planeBehaviour(void* passed)
{
	//These two lines of code just convert the void pointer to an int, non-pointer
	int* temp = passed;
	int index = *temp;
	
	//Let the program know we are done with the pointer
	sem_post(&pointerAccess);
	
	//Make sure we aren't quitting. . .
	while(numberString != "quit")
	{
		//Make sure the planes ready up in a random order by making them sleep for a random time
		int randomNumber = rand() % 10 + 1;
		sleep(randomNumber);
		
		//Make them wait until the runway is open
		sem_wait(&runway);
		
		//This should never happen, this is if the plane was readied when another is also already ready
		if(requestIndex > -1)
		{
			printf("Concurrency error, more than one plane landing or taking off\nCurrent request index is: %d\n", requestIndex);
			
			exit(0);
		}
		
		//Set the request index equal to this plane's index
		requestIndex = index;
	}
	
	//Exit the thread when done
	pthread_exit(NULL);
}

void *controlTowerBehaviour(void)
{
	//Make sure we don't want to exit
	while(numberString != "quit")
	{
		//If there is currently no plane trying to take off or land. . .
		if(requestIndex == -1)
		{
			//Let a plane take off or land
			sem_post(&runway);
			
			//ensure we don't post again by making requestIndex a new value
			requestIndex--;
		}
		//If there is a plane landing or taking off
		else if(requestIndex > -1 && requestIndex < PLANES)
		{
			//Get the proper delay 'planeStates[requestIndex]' is whether it is taking off or landing
			int seconds = planeStates[requestIndex] ? TAKEOFF_TIME : LANDING_TIME;
			
			//Get the proper action the plane is performing
			char* action = planeStates[requestIndex] ? "taking off" : "landing";
			
			//Print the action
			printf("Plane %d is %s\n", requestIndex, action);
			
			//Simulate the delay of the plane performing the action
			sleep(seconds);
			
			//Set the runway as open again
			requestIndex = -1;
		}
		//If there is an error with the index
		else if(requestIndex < -2 || requestIndex >= PLANES)
		{
			printf("Indexing error, requested index: %d", requestIndex);
			
			exit(0);
		}
	}
}

int main(void)
{
	//This initializes the random function
	srand(time(NULL));
	
	//This initializes the runway semaphore 
	//which is used to allow a plane to take off or land
	sem_init(&runway, 0, 0);
	
	//This initializes the pointerAccess semaphore 
	//which is used to ensure that the program waits until the plane thread is initialized
	sem_init(&pointerAccess, 0, 0);
	
	//This initailizes the request index 
	//which determines what plane is taking off or landing 
	//it contains the index of the plane, (-1 if none, and -2 if none and in ready mode)
	requestIndex = -1;
	
	//This loops through and creates as many planes as specified in the PLANES definition
	for(int i = 0; i < PLANES; i++)
	{
		//This simply makes every other plane grounded/in air
		//it reads, if i is odd, the plane is on the ground, i is even, the plane is in air
		planeStates[i] = i % 2 == 1 ? true : false;
		
		pthread_t planeThread;
		
		//If the thread doesn't get created properly. . .
		if(pthread_create(&planeThread, NULL, planeBehaviour, &i) != 0) //Creates thread here and passes in i
		{
			printf("Error making thread index: %d\n", i);
			
			exit(0);
		}
		
		//Waiting for the thread to finish up using the i pointer
		sem_wait(&pointerAccess);
	}
	
	//Run this thread as the control tower
	controlTowerBehaviour();
	
	//Exit the threads at the end of the program
	pthread_exit(NULL);
}