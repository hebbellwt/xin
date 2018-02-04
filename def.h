#include <time.h>


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

