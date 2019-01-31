 #ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "i8259.h"

#define RTC_REG_IDX_PORT 0x70
#define RTC_DATA_PORT 0x71
#define REG_A 0x8A
#define REG_B 0x8B
#define REG_C 0x8C
#define RATE_MASK 0xF0
volatile int32_t RTC_IR;

/* only use for initialize RTC */
#define RTC_INIT 0x42
#define RTC_IRQ 8
/*initialization of rtc */
void rtc_init(void);

/*rtc handler */
void rtc_handler(void);
int32_t rtc_write(int32_t fd, const uint8_t* buf, uint32_t nbytes);
int32_t rtc_read(int32_t fd, uint8_t* buf, uint32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
#endif /* _RTC_H */


