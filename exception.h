#ifndef EXCEPTION_H
#define EXCEPTION_H

extern void excep_divide_by_zero();
extern void excep_debug();
extern void excep_NMI();
extern void excep_breakpoint();
extern void excep_overflow();
extern void excep_bound_range();
extern void excep_invalid_opecode();
extern void excep_device_not_available();
extern void excep_double_fault();
extern void excep_coprocessor_segment_overrun();
extern void excep_invalid_tss();
extern void excep_segment_not_present();
extern void excep_stack_seg_fault();
extern void excep_general_protection();
extern void excep_page_fault();
extern void excep_floating_point();
extern void excep_alin_check();
extern void excep_machine_check();
extern void excep_simd_floating_point();
extern void excep_virtualization();
extern void excep_security();

#endif



