#ifndef MEM_DRAMSIM2_WRAPPER_H
#define MEM_DRAMSIM2_WRAPPER_H


#ifdef __cplusplus
extern "C"
{
#endif 

void* dramsim2_init(void *rFunc, void* wFunc, int size,
                    char * dram_ini_name, char * system_ini_name, char * ini_dir, char * logFileSuffix);
void memory_update(void* in_obj);
void memory_addtransaction(void* in_obj, unsigned char isWrite, unsigned long long  addr);
void memory_setCPUClockSpeed(void* in_obj, unsigned long long cpuClkFreqHz);
void memory_printstats(void* in_obj);
void memory_setBL(void* in_obj, unsigned int burstlength);
#ifdef __cplusplus
}
#endif



#endif
