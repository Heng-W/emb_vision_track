
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <mach/platform.h>
#include <mach/devices.h>

#include <asm/io.h>

#include "../mach/pwm.h"
#include "../mach/gpio.h"


#define LEFT_MOTOR_CH 2
#define RIGHT_MOTOR_CH 3



#define GPIO_PORT_N0 GPIOC_BASE_ADDR
#define GPIO_PIN_N0 29
#define GPIO_FUN_N0 0


#define GPIO_PORT_N1 GPIOC_BASE_ADDR
#define GPIO_PIN_N1 30
#define GPIO_FUN_N1 0



//10kHz PWM
#define PRESCALER (20 - 1)
#define DIVIDER    0      //2^(val) (val>4: external_tclk)
#define DURATION  (1000 - 1)
#define COMPARE   (50 - 1)


#define DEVICE_NAME "motor"

#define IOC_MAGIC 'x'



#define limit(val,min,max)  (val)<(min)?(min):((val)>(max)?(max):(val))


enum MOTOR_DEVICE_CMD
{
    SET_MOTOR,
    SET_DUTY,
    SET_CMP,
    SET_DUR,
    SET_PRES,
    SET_DIV,
    GET_COUNT,
    GET_CMP,
    GET_DUR,
    GET_PRES,
    GET_DIV,
    GET_FREQ,
    GET_DUTY,
    START_PWM,
    STOP_PWM,
    STOP_MOTOR
};



static volatile struct GPIO_REGS* port[2];
static const int GPIO_PIN[2] = {GPIO_PIN_N0, GPIO_PIN_N1};


static void set_motor(unsigned int ch, int val)
{
    val = limit(val, 1 - DURATION, DURATION);

    if (ch == 0)//left motor
    {
        if (val >= 0)
        {
            pwm_set_compare(LEFT_MOTOR_CH, val);
            gpio_set_low(port[0], GPIO_PIN[0]);
        }
        else
        {
            pwm_set_compare(LEFT_MOTOR_CH, DURATION - 1 + val);
            gpio_set_high(port[0], GPIO_PIN[0]);
        }
    }
    else//right motor
    {
        if (val >= 0)
        {
            pwm_set_compare(RIGHT_MOTOR_CH, val);
            gpio_set_low(port[1], GPIO_PIN[1]);
        }
        else
        {
            pwm_set_compare(RIGHT_MOTOR_CH, DURATION - 1 + val);
            gpio_set_high(port[1], GPIO_PIN[1]);
        }
    }
}

static long motor_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
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
    pwm_ch = (ch == 0) ? LEFT_MOTOR_CH : RIGHT_MOTOR_CH;
    num >>= 1;
    switch (num)
    {
        case SET_MOTOR:
            set_motor(ch, arg);
            break;
        case STOP_MOTOR:
            pwm_stop(pwm_ch);
            gpio_set_low(port[ch], GPIO_PIN[ch]);
            break;
        case SET_DUTY:
            ret = copy_from_user(&fdat, (float*)arg, 4);
            pwm_set_compare(pwm_ch, (unsigned int)(fdat * (pwm_get_duration(pwm_ch) + 1) / 100) - 1);
            break;
        case SET_CMP:
            pwm_set_compare(pwm_ch, arg);
            break;
        case SET_DUR:
            pwm_set_duration(pwm_ch, arg);
            break;
        case SET_PRES:
            pwm_set_prescaler(pwm_ch, arg);
            break;
        case SET_DIV:
            pwm_set_divider(pwm_ch, arg);
            break;
        case GET_COUNT:
            dat = pwm_get_count(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_CMP:
            dat = pwm_get_compare(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_DUR:
            dat = pwm_get_duration(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_PRES:
            dat = pwm_get_prescaler(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_DIV:
            dat = pwm_get_divider(pwm_ch);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_FREQ:
            dat = 2e8 / (pwm_get_prescaler(pwm_ch) + 1) / (1 << pwm_get_divider(pwm_ch)) / (pwm_get_duration(pwm_ch) + 1);
            ret = copy_to_user((unsigned int*)arg, &dat, 4);
            break;
        case GET_DUTY:
            fdat = 100.0f * (pwm_get_compare(pwm_ch) + 1) / (pwm_get_duration(pwm_ch) + 1);
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


static ssize_t motor_read(struct file* filp, char __user* buf, size_t count, loff_t* offp)
{

    return 0;
}

static ssize_t motor_write(struct file* filp, const char __user* buf, size_t count, loff_t* offp)
{

    return 0;
}


static int motor_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int motor_release(struct inode* inode, struct file* filp)
{
    return 0;
}

static struct file_operations motor_fops =
{
    .owner = THIS_MODULE,
    .open = motor_open,
    .release = motor_release,
    .read = motor_read,
    .write = motor_write,
    .unlocked_ioctl = motor_ioctl,
};

static struct miscdevice motor_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &motor_fops,
};


static void virt_addr_init(void)
{

    pwm_virt_addr_init();
    port[0] = ioremap(GPIO_PORT_N0, sizeof(struct GPIO_REGS));

#if GPIO_PORT_N0==GPIO_PORT_N1
    port[1] = port[0];
#else
    port[1] = ioremap(GPIO_PORT_N1, sizeof(struct GPIO_REGS));
#endif
}

static void virt_addr_release(void)
{
    pwm_virt_addr_release();
    iounmap(port[0]);
#if GPIO_PORT_N0 != GPIO_PORT_N1
    iounmap(port[1]);
#endif
}

static void gpio_init(void)
{
    gpio_set_func(port[0], GPIO_PIN[0], GPIO_FUN_N0);
    gpio_dir_out(port[0], GPIO_PIN[0]);
    gpio_set_low(port[0], GPIO_PIN[0]);

    gpio_set_func(port[1], GPIO_PIN[1], GPIO_FUN_N1);
    gpio_dir_out(port[1], GPIO_PIN[1]);
    gpio_set_low(port[1], GPIO_PIN[1]);
}


static int motor_init(void)
{
    int ret;
    virt_addr_init();

    pwm_config(LEFT_MOTOR_CH, PRESCALER, DIVIDER, DURATION, COMPARE);
    pwm_start(LEFT_MOTOR_CH);

    pwm_config(RIGHT_MOTOR_CH, PRESCALER, DIVIDER, DURATION, COMPARE);
    pwm_start(RIGHT_MOTOR_CH);

    gpio_init();

    ret = misc_register(&motor_dev);
    printk("%s: module init\n", DEVICE_NAME);
    return ret;
}

static void motor_exit(void)
{

    pwm_stop(LEFT_MOTOR_CH);
    pwm_stop(RIGHT_MOTOR_CH);

    gpio_set_low(port[0], GPIO_PIN[0]);
    gpio_set_low(port[1], GPIO_PIN[1]);

    virt_addr_release();
    misc_deregister(&motor_dev);
    printk("%s: module exit\n", DEVICE_NAME);
}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HW");
