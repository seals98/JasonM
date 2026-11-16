#ifndef SIM_H
#define SIM_H
#include <stdint.h>
extern uint8_t  sim_nor[];                 /* the whole QSPI NOR, as the XIP window would show it */
extern unsigned sim_errors;                /* protocol violations the model detected */
extern int      sim_xip;                   /* 1 = memory-mapped mode active */
extern unsigned sim_kicks;
void sim_nor_reset(void);                  /* chip-erase to 0xFF, clear counters */
#endif
