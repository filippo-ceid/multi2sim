#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <new>
#include <string>
#include "DRAMSim.h"

#include "dramsim2_wrapper.h"


using namespace DRAMSim;

class some_object
{
public:
	MultiChannelMemorySystem *memory;
	void (*readFunc)(unsigned, unsigned long long, unsigned long long);
	void (*writeFunc)(unsigned, unsigned long long, unsigned long long);

	void init(void* rFunc, void* wFunc);
	void read_complete(unsigned, uint64_t, uint64_t);
	void write_complete(unsigned, uint64_t, uint64_t);
};




/* callback functors */
void some_object::read_complete(unsigned id, uint64_t address, uint64_t clock_cycle)
{
	readFunc(id,address,clock_cycle);
}

void some_object::write_complete(unsigned id, uint64_t address, uint64_t clock_cycle)
{
	writeFunc(id,address,clock_cycle);
}

/* FIXME: this may be broken, currently */
void power_callback(double a, double b, double c, double d)
{
//      printf("power callback: %0.3f, %0.3f, %0.3f, %0.3f\n",a,b,c,d);
}


void some_object::init(void* rFunc, void* wFunc)
{
	readFunc = (void (*)(unsigned, unsigned long long, unsigned long long))rFunc;
	writeFunc = (void (*)(unsigned, unsigned long long, unsigned long long))wFunc;

	TransactionCompleteCB *read_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(this, &some_object::read_complete);
        TransactionCompleteCB *write_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(this, &some_object::write_complete);

        MultiChannelMemorySystem *mem = getMemorySystemInstance("ini/DDR2_micron_16M_8b_x8_sg3E.ini", "ini/system.ini", "/home/ray/WORK/multi2sim", "example_app", 16384);


        mem->RegisterCallbacks(read_cb, write_cb, power_callback);
        this->memory = mem;
}


extern "C"
{

void* dramsim2_init(void *rFunc, void* wFunc)
{
	some_object * obj = (some_object*)malloc(sizeof(some_object));
	obj->init(rFunc,wFunc);

	return (void*) obj;
}

void memory_update(void* in_obj)
{
	some_object * obj = (some_object*)in_obj;
	obj->memory->update();
}

void memory_addtransaction(void* in_obj, char isWrite, unsigned long long addr)
{
	some_object * obj = (some_object*)in_obj;

	if (isWrite==1)
		obj->memory->addTransaction(true, addr);
	else
		obj->memory->addTransaction(false, addr);
}

}



