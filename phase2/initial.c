/*
	This module implements main() and exports the nucleus's global 
	variables. (E.G. Process Count, device semaphores, etc.)
	Contains the bootstrap code that is run on machine startup
*/

#include "../e/asl.e"
#include "../e/pcb.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"
#include "../h/const.h"
#include "../h/types.h"
#include "p2test.e"

/* global variables */
int procCount, sftBlkCount;
pcb_PTR currProc;
pcb_PTR readyQueue;

/* does the things */
int main(){
	int i;
	unsigned int RAMTOP;
	state_PTR newLocation;
	devregarea_t* deviceInfo = (devregarea_t *) 0x10000000;

	/* calcualte RAMTOP */
	RAMTOP = (deviceInfo -> rambase) + (deviceInfo -> ramsize);

	/* Initialize the PCB and ASL lists */
	initPcbs();
	initASL();

	/* initialize global variables */
	readyQueue = mkEmptyProcQ();
	currProc = NULL;
	procCount = sftBlkCount = 0;

	/* Initialize 4 "new" interrupt vectors */
	/* syscall */
	newLocation =  (state_PTR)SYSCALLNEWAREA;
	newLocation -> s_sp = RAMTOP;
	newLocation -> s_status = ALLOFF;
	newLocation -> s_pc = (memaddr) syscallHandler;
	newLocation -> s_t9 = (memaddr) syscallHandler;

	/* pgmtrp */
	newLocation =  (state_PTR)PBGTRAPNEWAREA;
	newLocation -> s_sp = RAMTOP;
	newLocation -> s_status = ALLOFF;
	newLocation -> s_pc = (memaddr) pgmTrap;
	newLocation -> s_t9 = (memaddr) pgmTrap;

	/* tlbmgmt */
	newLocation =  (state_PTR)TBLMGMTNEWAREA;
	newLocation -> s_sp = RAMTOP;
	newLocation -> s_status = ALLOFF;	
	newLocation -> s_pc = (memaddr) tlbManager;
	newLocation -> s_t9 = (memaddr) tlbManager;
	
	/* interrupt */
	newLocation =  (state_PTR)INTPNEWAREA;
	newLocation -> s_sp = RAMTOP;
	newLocation -> s_status = ALLOFF;	
	newLocation -> s_pc = (memaddr) interruptHandler;
	newLocation -> s_t9 = (memaddr) interruptHandler;

	/* create a semD for each device and set to 0 */
	int semD[DEVINTNUM * DEVPERINT];
	for(i = 0; i < (DEVINTNUM * DEVPERINT); ++i){
		semD[i] = 0;
	}

	/* create an initial process */	
	currProc = allocPcb();
	currProc -> p_s.s_sp = (RAMTOP - PAGESIZE);
	currProc -> p_s.s_pc = (memaddr)test;
	currProc -> p_s.s_t9 = (memaddr)test;
	currProc -> p_s.s_status = ALLOFF; /* not sure what to set this to*/

	/* insert first process into readyQ */
	insertProcQ(&readyQueue, currProc);

	/* send control over to the scheduler */
	scheduler();

	return 1;
}