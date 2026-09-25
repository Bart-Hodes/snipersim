#ifndef __PIMSIM_HOOK_H
#define __PIMSIM_HOOK_H

// Bridge between the simulated host application and pimsim (the PIM device
// simulator). The application issues SimUser(PIMSIM_CMD_*, arg) magic
// instructions; this hook services them without simulating any application
// instructions and charges the host core with PIM stalls through
// DelayInstruction::PIM_WAIT.
namespace PimsimHook
{
   void init();
}

#endif // __PIMSIM_HOOK_H
