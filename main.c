#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>  
#include <fcntl.h>
#include <pthread.h>  


#define CLOCKID CLOCK_REALTIME 

typedef struct tagCoffee
{
	char *size;
	int price;
} Coffee;

typedef enum tagVmState
{
	VM_IDLE = 0,
	VM_SIZE_SELECT,
	VM_OFF
} VmState;

typedef struct tagVendingMachine
{
	VmState state;
	int temperature;
	int price;
	int coin_sum;
	timer_t coin_insert_timerid;
} VendingMachine;

Coffee coffees[] = {
		{"small", 50},
		{"medium", 100},
		{"big", 150}
};

void vm_clear_stdin()
{
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}

void vm_menu()
{
	printf("=============size============\n");
	printf("small       medium        big\n");
	printf("note: press cancel at anytime\n");
	printf("note: press off to shut down\n");
}

void vm_reset_to_idle(VendingMachine* vm)
{
	vm->state = VM_IDLE;
	vm->coin_sum = 0;
	vm->price = 0;

	if ((long)vm->coin_insert_timerid)
	{
		struct itimerspec it;
		it.it_value.tv_sec = 0;   
		it.it_value.tv_nsec = 0;
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = 0;
		timer_settime(vm->coin_insert_timerid, 0, &it, NULL);
	}

	vm_menu();
}

void vm_init(VendingMachine* vm)
{
	vm->temperature = 65;
	memset(&(vm->coin_insert_timerid), 0, sizeof(timer_t));
	vm_reset_to_idle(vm);
}

void timer_coin_insert_callback(union sigval v)  
{  
	VendingMachine* vm = (VendingMachine*)v.sival_ptr;
	printf("insert coin time out\ncoin %d is returned to you \n", vm->coin_sum);
	vm_reset_to_idle(vm);
}

void timer_coin_insert_set(VendingMachine* vm)
{

	if (0 == (long)vm->coin_insert_timerid)
	{
		struct sigevent evp;  
		memset(&evp, 0, sizeof(struct sigevent));
		evp.sigev_value.sival_ptr = vm;
		evp.sigev_notify = SIGEV_THREAD;
		evp.sigev_notify_function = timer_coin_insert_callback;
		timer_create(CLOCKID, &evp, &(vm->coin_insert_timerid));
	}

	struct itimerspec it;
	it.it_value.tv_sec = 120;   
	it.it_value.tv_nsec = 0;
	it.it_interval.tv_sec = 120;
	it.it_interval.tv_nsec = 0;
	timer_settime(vm->coin_insert_timerid, 0, &it, NULL);
}


void check_price(VendingMachine* vm, char* str)
{
	int i=0;
	
	for (i=0; i<3; i++) 
	{
		if (0 == strcmp(str, coffees[i].size)){
			vm->price = coffees[i].price;
			break;
		}
	}
	if (vm->price)
	{	
		printf("size %s selected\ninsert %d cents now\n", str, vm->price);
		vm->state = VM_SIZE_SELECT;
	}
	else
	{
		printf("wrong size, big, medium or small?\n");
	}
}

void check_coin(VendingMachine* vm, char* str)
{
	int coin = 0;
	if (!strcmp(str, "cancel")) 
	{
		printf("size selection canceled\n coin %d is returned to you \n", vm->coin_sum);
		vm_reset_to_idle(vm);	
		return;
	}	

	coin = atoi(str);
	if (10 == coin || 20 == coin || 50 == coin || 100 ==coin)
	{
		vm->coin_sum += coin;
		timer_coin_insert_set(vm);
		if (vm->coin_sum < vm->price)
			printf("%c", '\007');
	}
	else
	{
		printf("wrong coin\n");
	}
	printf("coin sum %d inserted \n", vm->coin_sum);

	if (vm->coin_sum >= vm->price)
	{
		if ((long)vm->coin_insert_timerid)
		{
			struct itimerspec it;
			it.it_value.tv_sec = 0;   
			it.it_value.tv_nsec = 0;
			it.it_interval.tv_sec = 0;
			it.it_interval.tv_nsec = 0;
			timer_settime(vm->coin_insert_timerid, 0, &it, NULL);
		}
		//wait for cooking
		while(vm->temperature <= 75);
			printf("please take your coffee\nthank you for buying!\n");
		vm_reset_to_idle(vm);
	}			
	return;
}

void check_power_off(VendingMachine* vm, char* str)
{
	if (!strcmp(str, "off")) 
	{
		vm->state = VM_OFF;
	}	
}

void* pthread_func_temp_control (void* input)    
{  
	VendingMachine* vm = (VendingMachine*)input;
	while (VM_OFF != vm->state)
	{
		if (vm->price <= vm->coin_sum && vm->price)
		{
			if(vm->temperature <= 75)
			{
				printf("heating, current temp = %d ...\n", vm->temperature);
				vm->temperature ++;
			}
		}
		else
		{
			if(vm->temperature <= 70)
			{
				printf("heating, current temp = %d ...\n", vm->temperature);
				vm->temperature ++;
			}
			else if (vm->temperature > 70)
			{
				printf("cooling, current temp = %d ...\n", vm->temperature);
				vm->temperature --;
			}
			
		}
		sleep(5);
	}
}


void* pthread_func_input (void* input)    
{  
	char* buf = (char*)input;
	while(scanf("%10s", buf))
	{
		vm_clear_stdin();
		sleep(1);
	}
}

int main(int argc, const char* argv[])
{	
	VendingMachine vm = {0};
	char keyboard_input[10] = {0};
	pthread_t pthread_input = 0;
	pthread_t pthread_temp_control = 0;

	vm_init(&vm);
	pthread_create(&pthread_temp_control, NULL, pthread_func_temp_control, (void*)&vm);
	pthread_create(&pthread_input, NULL, pthread_func_input, (void*)keyboard_input);

	while(VM_OFF != vm.state)
	{
		if(keyboard_input[0])
		{	
			check_power_off(&vm, keyboard_input);
			switch(vm.state)
			{
				case VM_IDLE:
					check_price(&vm, keyboard_input);
					break;
				case VM_SIZE_SELECT:
					check_coin(&vm, keyboard_input);
					break;
				case VM_OFF:
					printf("clean....\npower off\nsee you!\n");
					break;
				default:
					break;			
			}
			keyboard_input[0] = 0;
		}
	}		
}
