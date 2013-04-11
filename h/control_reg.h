/* control_reg.h */
#ifndef _CONTROLREG_H_
#define _CONTROLREG_H_

void enable_paging();

unsigned long read_cr0();
unsigned long read_cr2();
unsigned long read_cr3();
unsigned long read_cr4();
unsigned long get_PDBR();

void write_cr0(unsigned long n);
void write_cr3(unsigned long n);
void write_cr4(unsigned long n);
void set_PDBR(unsigned long n);

#endif
