
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <mach/platform.h>
#include <mach/devices.h>

#include <asm/io.h>

#include "../mach/pwm.h"


#define SERVO0_PWM_CH 0
#define SERVO1_PWM_CH 1


#define DEFAULT_ANGLE0  90

#define DEFAULT_ANGLE1  90


//50Hz PWM
#define PRESCALER (200 - 1)
#define DIVIDER    0      //2^(val) (val>4: external_tclk)
#define DURATION  (20000 - 1)



#define DEVICE_NAME "servo"

#define IOC_MAGIC 0x81



#define limit(val,min,max)  (val)<(min)?(min):((val)>(max)?(max):(val))


enum SERVO_DEVICE_CMD
{
    SET_ANGLE,
    GET_ANGLE,
    GET_FREQ,
    GET_DUTY,
    START_PWM,
    STOP_PWM
};


static float angle2duty(float angle)
{
    angle = limit(angle, 0, 180);
    return 2.5 + angle / 18;
}

static float duty2angle(float duty)
{
    duty = limit(duty, 2.5, 12.5);
    return (duty - 2.5) * 18;

}

static unsigned int angle2cmp(float angle)
{
    return (unsigned int)(200 * angle2duty(angle));
}

static float cmp2angle(unsigned int cmp)
{
    return duty2angle((float)cmp / 200);

}

static long servo_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    unsigned int dat = 0;
    float fdat = 0;
    int ret = 0;
    unsigned int num, ch;
    unsigned int pwm_ch;
    //检验幻数
    if (_IOC_TYPE(cmd) != IOC_MAGIC)
    {
        return -ENODEV;
    }
    //提取序数
    num = _IOC_NR(cmd);
    ch = num & 0x1;
    pwm_ch = (ch == 0) ? SERVO0_PWM_CH : SERVO1_PWM_CH;
    num >>= 1;
    switch (num)
    {
        case SET_ANGLE:
            ret = copy_from_user(&fdat, (float*)arg, 4);
            pwm_set_compare(pwm_ch, angle2cmp(fdat));
            break;
        case GET_ANGLE:
            fdat = cmp2angle(pwm_get_compare(pwm_ch));
            ret = copy_to_user((float*)arg, &fdat, 4);
            break;
        case GET_FREQ:
            dat = pwm_get_prescaler(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_DUTY:
            fdat = (float)arg / 200;
            ret = copy_to_user((float*)arg, &fdat, 4);
            break;
        case START_PWM:
            pwm_start(pwm_ch);
            break;
        case STOP_PWM:
            pwm_stop(pwm_ch);
            break;
        default:
            break;
    }
    if (ret == 0)
        return 0;
    return -EMSGSIZE;
}


static ssize_t servo_read(struct file* filp, char __user* buf, size_t count, loff_t* offp)
{

    return 0;
}

static ssize_t servo_write(struct file* filp, const char __user* buf, size_t count, loff_t* offp)
{

    return 0;
}

static int servo_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int servo_release(struct inode* inode, struct file* filp)
{
    return 0;
}

static struct file_operations servo_fops =
{
    .owner = THIS_MODULE,
    .open = servo_open,
    .release = servo_release,
    .read = servo_read,
    .write = servo_write,
    .unlocked_ioctl = servo_ioctl,
};

static struct miscdevice servo_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &servo_fops,
};


static int servo_init(void)
{
    int ret;
    pwm_virt_addr_init();

    pwm_config(SERVO0_PWM_CH, PRESCALER, DIVIDER, DURATION, angle2cmp(DEFAULT_ANGLE0));
    pwm_start(SERVO0_PWM_CH);

    pwm_config(SERVO1_PWM_CH, PRESCALER, DIVIDER, DURATION, angle2cmp(DEFAULT_ANGLE1));
    pwm_start(SERVO1_PWM_CH);

    ret = misc_register(&servo_dev);
    printk("%s: module init\n", DEVICE_NAME);
    return ret;
}

static void servo_exit(void)
{
    pwm_stop(SERVO0_PWM_CH);
    pwm_stop(SERVO1_PWM_CH);

    pwm_virt_addr_release();

    misc_deregister(&servo_dev);
    printk("%s: module exit\n", DEVICE_NAME);
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HW");
