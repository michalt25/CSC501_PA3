/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


extern int page_replace_policy;
/*
 * srpolicy - set page replace policy.
 *
 * Note: It has been guaranteed to us in the project
 *       spec that this won't be changed on us in the 
 *       middle of a run. It will be set at the beginning
 *       and won't be touched again. 
 */
SYSCALL srpolicy(int policy) {

    // Make sure the policy they give is valid
    if ((policy != FIFO) && (policy != AGING))
        return SYSERR;

    // Set the debugTA variable. This will be used to turn on 
    // debugging output for TAs to use during grading.
    debugTA = 1;

    // Set the policy and return
    page_replace_policy = policy;
    return OK;
}

/*
 * grpolicy - get page replace policy 
 */
SYSCALL grpolicy() {
  return page_replace_policy;
}
