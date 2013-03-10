#ifndef MEM_DRAMSIM2_WRAPPER_H
#define MEM_DRAMSIM2_WRAPPER_H


#ifdef __cplusplus
extern "C"
{
#endif 

void* dramsim2_init(void *rFunc, void* wFunc);
void memory_update(void* in_obj);
void memory_addtransaction(void* in_obj, char isWrite, unsigned long long  addr);



#ifdef __cplusplus
}
#endif



#endif
