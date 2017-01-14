#include <stdio.h> /* printf */
#include <linux/rtc.h> /* struct rtc_time */
#include <sys/ioctl.h> /* ioctl */
#include <fcntl.h> /* O_RDONLY */
#include <unistd.h> /* close/fork */
#include <errno.h> /* errno */
#include <string.h> /* strerror */
#include <pthread.h> /* pthread_mutex_lock */
#include <assert.h>  /* assert */

#define _toString(x) #x
#define toString(x) _toString(x)
#define LOG_ERR(...) do{ fprintf(stderr, "file: " __FILE__ " line:" toString(__LINE__) " "__VA_ARGS__); }while(0)

static pthread_mutex_t rtc_mutex = PTHREAD_MUTEX_INITIALIZER;
static void rtc_lock(void);
static void rtc_unlock(void);
static void rtc_time_dump(struct rtc_time *rt);
static void* unit_test_entry(void *arg);

static int RTC_ALM_READ_test(void);
static int RTC_AIE_ON_test(void);
static int RTC_AIE_OFF_test(void);
static int RTC_UIE_ON_test(void);
static int RTC_UIE_OFF_test(void);
static int RTC_PIE_ON_test(void);
static int RTC_PIE_OFF_test(void);
static int RTC_WIE_ON_test(void);
static int RTC_WIE_OFF_test(void);
static int RTC_ALM_SET_test(void);
static int RTC_RD_TIME_test(void);
static int RTC_SET_TIME_test(void);
static int RTC_IRQP_READ_test(void);
static int RTC_IRQP_SET_test(void);
static int RTC_EPOCH_READ_test(void);
static int RTC_EPOCH_SET_test(void);
static int RTC_WKALM_SET_test(void);
static int RTC_WKALM_RD_test(void);
static int RTC_PLL_GET_test(void);
static int RTC_PLL_SET_test(void);
static int RTC_VL_READ_test(void);
static int RTC_VL_CLR_test(void);

int main(int argc, char *argv[])
{
    const int RTC_THREAD_NUM = 10;
    pthread_t rtc_thread_table[RTC_THREAD_NUM];
    int err;
    int i;
    
    do {
	for (i = 0; i < RTC_THREAD_NUM; i++) {
	    err = pthread_create(&rtc_thread_table[i], NULL, unit_test_entry, NULL);
	    printf("err = %d\n", err);
	    printf("rtc_thread = 0x%x\n", (int)rtc_thread_table[i]);
	}

	for (i = 0; i < RTC_THREAD_NUM; i++) {
	    pthread_join(rtc_thread_table[i], NULL);
	}
    } while(0);

    return 0;
}

/*
 * lock /dev/rtc
 */
static void rtc_lock()
{
    pthread_mutex_lock(&rtc_mutex);
}

/*
 * unlock /dev/rtc
 */
static void rtc_unlock()
{
    pthread_mutex_unlock(&rtc_mutex);
}

/*
 * 打印 struct rtc_time 各个成员的值
 */
static void rtc_time_dump(struct rtc_time *rt)
{
	pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&dump_mutex);
	printf("...........................\n");
#define DUMP(x) printf(#x":%d\n", rt->x)
	DUMP(tm_sec);
	DUMP(tm_min);
	DUMP(tm_hour);
	DUMP(tm_mday);
	DUMP(tm_mon);
	DUMP(tm_year);
	DUMP(tm_wday);
	DUMP(tm_yday);
	DUMP(tm_isdst);
#undef DUMP
	printf("###########################\n");
	pthread_mutex_unlock(&dump_mutex);
}

/*
 * RTC unit test entry
 */
static void* unit_test_entry(void *arg)
{
    (void)arg;

    assert(RTC_AIE_ON_test() == 0);
    assert(RTC_AIE_OFF_test() == 0);
    assert(RTC_UIE_ON_test() == 0);
    assert(RTC_UIE_OFF_test() == 0);
    assert(RTC_PIE_ON_test() == 0);
    assert(RTC_PIE_OFF_test() == 0);
    assert(RTC_WIE_ON_test() == ENOTTY);
    assert(RTC_WIE_OFF_test() == ENOTTY);
    assert(RTC_ALM_SET_test() == 0);
    assert(RTC_ALM_READ_test() == 0);
    assert(RTC_RD_TIME_test() == 0);
    assert(RTC_SET_TIME_test() == 0);
    assert(RTC_IRQP_READ_test() == 0);
    assert(RTC_IRQP_SET_test() == 0);
    assert(RTC_EPOCH_READ_test() == ENOTTY);
    assert(RTC_EPOCH_SET_test() == ENOTTY);
    assert(RTC_WKALM_SET_test() == 0);
    assert(RTC_WKALM_RD_test() == 0);
    assert(RTC_PLL_GET_test() == ENOTTY);
    assert(RTC_PLL_SET_test() == ENOTTY);
    assert(RTC_VL_READ_test() == ENOTTY);
    assert(RTC_VL_CLR_test() == ENOTTY);

    return NULL;
}

/*
 * RTC_AIE_ON test
 * Alarm interrupt. enable on
 */
static int RTC_AIE_ON_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_AIE_ON, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_AIE_ON, 0) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();

    return err;
}

/*
 * RTC_AIE_OFF test
 * Alarm interrupt. disable off
 */
static int RTC_AIE_OFF_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_AIE_OFF, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_AIE_OFF, 0) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();   
    
    return err;
}

/*
 * RTC_UIE_ON test
 * Update int. enable on
 */
static int RTC_UIE_ON_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_UIE_ON, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_AIE_OFF, 0) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();   
    
    return err;
}

/*
 * RTC_UIE_OFF test
 * Update interrupt. disable off
 */
static int RTC_UIE_OFF_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_UIE_OFF, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_UIE_OFF, 0) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();   
    
    return err;   
}

/*
 * RTC_PIE_ON test
 * Periodic interrupt. enable on
 */
static int RTC_PIE_ON_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_PIE_ON, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_PIE_ON, 0) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();   
    
    return err; 
}

/*
 * RTC_PIE_OFF test
 * Periodic interrupt. disable off
 */
static int RTC_PIE_OFF_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_PIE_OFF, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_PIE_OFF, 0) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while (0);
    rtc_unlock();    
    
    return err;
}

/*
 * RTC_WIE_ON test
 * Watchdog interrupt. enable on
 */
static int RTC_WIE_ON_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_WIE_ON, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_WIE_ON, 0) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd);
    } while (0);
    rtc_unlock();    
    
    return err;
}

/*
 * RTC_WIE_OFF test
 * Watchdog interrupt. disable off
 */
static int RTC_WIE_OFF_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();    
    do {
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_WIE_OFF, 0);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_WIE_OFF, 0) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd);
    } while (0);
    rtc_unlock();  

    return err;
}

/*
 * RTC_ALM_SET test
 * Set alarm time
 */
static int RTC_ALM_SET_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();

    do {
	int fd;
	struct rtc_time rt;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	rt.tm_hour = 0;
	rt.tm_min  = 0;
	rt.tm_sec  = 0;
	rt.tm_year = 1900;
	rt.tm_mon  = 0;
	rt.tm_mday = 0;
	rt.tm_isdst = 0;
	err = ioctl(fd, RTC_ALM_SET, &rt);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_ALM_SET, &rt) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	rtc_time_dump(&rt);
	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_ALM_READ test
 * Read alarm time
 */
static int RTC_ALM_READ_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();

    do {
	int fd;
	struct rtc_time rt;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_ALM_READ, &rt);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_ALM_READ, &rt) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	rtc_time_dump(&rt);
	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_RD_TIME test
 * Read RTC time
 */
static int RTC_RD_TIME_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();

    do {
	int fd;
	struct rtc_time rt;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_RD_TIME, &rt);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_RD_TIME, &rt) failed\n");
	    err = errno;
	    close(fd);
	    break;
	}

	rtc_time_dump(&rt);
	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_SET_TIME test
 * Set RTC time
 */
static int RTC_SET_TIME_test()
{
    const char *RTC_DEV = "/dev/rtc";
    int err = 0;

    rtc_lock();

    do {
	int fd;
	struct rtc_time rt;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	rt.tm_hour = 9;
	rt.tm_min  = 50;
	rt.tm_sec  = 4;
	rt.tm_year = 2017 - 1900; /* can't bigger than 1900 */
	rt.tm_mon  = 1;
	rt.tm_mday = 13;
	err = ioctl(fd, RTC_SET_TIME, &rt);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_SET_TIME, &rt) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	rtc_time_dump(&rt);
	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_IRQP_READ test
 * Read IRQ rate
 */
static int RTC_IRQP_READ_test()
{
    int err = 0;

    rtc_lock();

    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	unsigned long irq_rate = 0; /* must be 'unsigned long' */

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_IRQP_READ, &irq_rate);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_IRQP_READ, &irq_freq) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	printf("RTC IRQ rate:%ld\n", irq_rate);
	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_IRQP_SET test
 * Set IRQ rate
 */
static int RTC_IRQP_SET_test()
{
    int err = 0;

    rtc_lock();

    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	unsigned long new_irq_rate = 512; /* must be 'unsigned long' */

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_IRQP_SET, new_irq_rate); /* not &new_irq_rate */
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_IRQP_SET, new_irq_rate) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_EPOCH_READ test
 * Read epoch
 */
static int RTC_EPOCH_READ_test()
{
    int err = 0;

    rtc_lock();

    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	unsigned long epoch = 0; /* must be 'unsigned long' */

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_EPOCH_READ, &epoch);
	if (err) {
	    LOG_ERR("err = ioctl(fd, RTC_EPOCH_READ, new_irq_rate) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}
	printf("EPOCH:%ld\n", epoch);

	close(fd); 
    } while(0);

    rtc_unlock();
    
    return err;
}

/*
 * RTC_EPOCH_SET test
 * Set epoch
 */
static int RTC_EPOCH_SET_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	unsigned long new_epoch = 986; /* must be 'unsigned long' */

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_EPOCH_SET, &new_epoch);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_EPOCH_SET, &new_epoch) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_WKALM_SET test
 * Set wakeup alarm
 */
static int RTC_WKALM_SET_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	struct rtc_wkalrm rt_alarm;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	rt_alarm.enabled = 1;
	rt_alarm.pending = 0;
	rt_alarm.time.tm_hour = 9;
	rt_alarm.time.tm_min  = 0;
	rt_alarm.time.tm_sec  = 0;
	rt_alarm.time.tm_year = 2017 - 1900;
	rt_alarm.time.tm_mon  = 1;
	rt_alarm.time.tm_mday = 13;

	err = ioctl(fd, RTC_WKALM_SET, &rt_alarm);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_WKALM_SET, &rt_alarm) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_WKALM_RD test
 * Get wakeup alarm
 */
static int RTC_WKALM_RD_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	struct rtc_wkalrm rt_alarm;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_WKALM_RD, &rt_alarm);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_WKALM_RD, &rt_alarm) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}
	printf("rt_alarm.enabled:%d rt_alarm.pending:%d\n",
		rt_alarm.enabled, rt_alarm.pending);
	rtc_time_dump(&rt_alarm.time);

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_PLL_GET test
 * Get PLL correction
 */
static int RTC_PLL_GET_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	struct rtc_pll_info rtc_pll;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	err = ioctl(fd, RTC_PLL_GET, &rtc_pll);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_PLL_GET, &rtc_pll) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	printf("pll_ctrl:%d\n", rtc_pll.pll_ctrl);
	printf("pll_value:%d\n", rtc_pll.pll_value);
	printf("pll_max:%d\n", rtc_pll.pll_max);
	printf("pll_min:%d\n", rtc_pll.pll_min);
	printf("pll_posmult:%d\n", rtc_pll.pll_posmult);
	printf("pll_negmult:%d\n", rtc_pll.pll_negmult);
	printf("pll_clock:%ld\n", rtc_pll.pll_clock);

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_PLL_SET test
 * 
 */
static int RTC_PLL_SET_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	struct rtc_pll_info new_rtc_pll;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}

	new_rtc_pll.pll_clock = 1;
	new_rtc_pll.pll_ctrl  = 2;
	new_rtc_pll.pll_max   = 8;
	new_rtc_pll.pll_min   = 4;
	
	err = ioctl(fd, RTC_PLL_SET, &new_rtc_pll);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_PLL_GET, &rtc_pll) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_VL_READ test
 * Voltage low detector
 */
static int RTC_VL_READ_test()
{
    int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;
	int voltage_low_dector;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}
	
	err = ioctl(fd, RTC_VL_READ, &voltage_low_dector);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_VL_READ, &voltage_low_dector) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}

/*
 * RTC_VL_CLR test
 */
static int RTC_VL_CLR_test()
{
   int err = 0;

    rtc_lock();
    do {
	const char *RTC_DEV = "/dev/rtc";
	int fd;

	fd = open(RTC_DEV, O_RDONLY);
	if (fd < 0) {
	    LOG_ERR("fd = open(RTC_DEV, O_RDONLY) failed:%s\n", strerror(errno));
	    err = errno;
	    break;
	}
	
	err = ioctl(fd, RTC_VL_CLR, 0);
	if (err) {
	    LOG_ERR("ioctl(fd, RTC_VL_CLR, 0) failed:%s\n", strerror(errno));
	    err = errno;
	    close(fd);
	    break;
	}

	close(fd); 
    } while(0);
    rtc_unlock();
    
    return err;
}