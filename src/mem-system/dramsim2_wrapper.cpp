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

	void init(void* rFunc, void* wFunc, int size,
                  char * dram_ini_name, char * system_ini_name, char * ini_dir, char * logFileSuffix);
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


void some_object::init(void* rFunc, void* wFunc, int size,
                       char * dram_ini_name, char * system_ini_name, char * ini_dir, char * logFileSuffix)
{
	readFunc = (void (*)(unsigned, unsigned long long, unsigned long long))rFunc;
	writeFunc = (void (*)(unsigned, unsigned long long, unsigned long long))wFunc;

	TransactionCompleteCB *read_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(this, &some_object::read_complete);
        TransactionCompleteCB *write_cb = new Callback<some_object, void, unsigned, uint64_t, uint64_t>(this, &some_object::write_complete);

        MultiChannelMemorySystem *mem = getMemorySystemInstance(dram_ini_name, system_ini_name, ini_dir, "dram_result", size, logFileSuffix);


        mem->RegisterCallbacks(read_cb, write_cb, power_callback);
        this->memory = mem;
}


extern "C"
{

void* dramsim2_init(void *rFunc, void* wFunc, int size,
                    char * dram_ini_name, char * system_ini_name, char * ini_dir, char * logFileSuffix)
{
	some_object * obj = (some_object*)malloc(sizeof(some_object));
	obj->init(rFunc,wFunc, size, dram_ini_name, system_ini_name, ini_dir, logFileSuffix);

	return (void*) obj;
}

void memory_update(void* in_obj)
{
	some_object * obj = (some_object*)in_obj;
	obj->memory->update();
}

void memory_addtransaction(void* in_obj, unsigned char isWrite, unsigned long long addr)
{
	some_object * obj = (some_object*)in_obj;

	if (isWrite==1)
		obj->memory->addTransaction(true, addr);
	else
		obj->memory->addTransaction(false, addr);
}

void memory_addtransaction_priority(void* in_obj, unsigned char isWrite, unsigned long long addr, unsigned char priority)
{
	some_object * obj = (some_object*)in_obj;

	if (isWrite==1)
		obj->memory->addTransaction(true, addr, priority);
	else
		obj->memory->addTransaction(false, addr, priority);
}

void memory_setCPUClockSpeed(void* in_obj, unsigned long long cpuClkFreqHz)
{
      some_object * obj = (some_object*)in_obj;
      obj->memory->setCPUClockSpeed(cpuClkFreqHz);

      return;
}
void memory_printstats(void* in_obj)
{
      some_object * obj = (some_object*)in_obj;
      obj->memory->printStats(true);

      return;
}
void memory_setBL(void* in_obj, unsigned int burstlength)
{
      some_object * obj = (some_object*)in_obj;
      obj->memory->setBL(burstlength);

      return;
}

}



