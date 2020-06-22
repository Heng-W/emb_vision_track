/*
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <mach/platform.h>
#include <mach/devices.h>
*/
#include "pwm.h"
#include <asm/io.h>



#define GPIOC_BASE_ADDR  (0xC001C000)
#define GPIOD_BASE_ADDR  (0xC001D000)

#define PWM_BASE_ADDR    (0xC0018000)


struct GPIO_REGS
{
    unsigned int OUT;
    unsigned int OUTENB;//0:input  1:output
    unsigned int RESERVED[6];
    unsigned int ALTFN0;//0-15pin
    unsigned int ALTFN1;//16-31pin
};

struct PWM_CHx_REGS
{
    unsigned int TCNTB;
    unsigned int TCMPB;
    unsigned int TCNTO;
};

struct PWM_REGS
{
    unsigned int TCFG0;
    unsigned int TCFG1;
    unsigned int TCON;
    struct PWM_CHx_REGS CH[4];
};

static volatile struct GPIO_REGS* GPIOC;
static volatile struct GPIO_REGS* GPIOD;
static volatile struct PWM_REGS* PWM;
static int ref_cnt;


static void gpio_get_msg(unsigned int i, volatile struct GPIO_REGS** port, unsigned int* pin)
{
    switch (i)
    {
        case 0:
            *port = GPIOD;
            *pin = 1;
            break;
        case 1:
            *port = GPIOC;
            *pin = 13;
            break;
        case 2:
            *port = GPIOC;
            *pin = 14;
            break;
        case 3:
            *port = GPIOD;
            *pin = 0;
            break;
        default:
            break;
    }
}

static void gpio_set_func(unsigned int i)
{
    volatile struct GPIO_REGS* port = NULL;
    unsigned int pin = 0;
    int fn = (i == 0) ? 1 : 2;

    gpio_get_msg(i, &port, &pin);

    if (pin < 16)
    {
        port->ALTFN0 &= ~(3 << (pin * 2));
        port->ALTFN0 |= (fn << (pin * 2));
    }
    else if (pin < 32)
    {
        port->ALTFN1 &= ~(3 << ((pin - 16) * 2));
        port->ALTFN1 |= (fn << ((pin - 16) * 2));
    }
}

void gpio_reset(unsigned int i)
{
    volatile struct GPIO_REGS* port = NULL;
    unsigned int pin = 0;

    gpio_get_msg(i, &port, &pin);

    port->OUTENB |= (1 << pin);
    port->OUT &= ~(1 << pin);
}

static void gpio_config(unsigned int i)
{
    gpio_set_func(i);
    gpio_reset(i);
}

void pwm_set_prescaler(unsigned int i, unsigned int value)
{
    value &= 0xff;
    if (i < 2)
    {
        PWM->TCFG0 &= ~0xff;
        PWM->TCFG0 |= value;
    }
    else
    {
        PWM->TCFG0 &= ~(0xff << 8);
        PWM->TCFG0 |= (value << 8);
    }
}

void pwm_set_divider(unsigned int i, unsigned int value)
{
    value &= 0xf;
    PWM->TCFG1 &= ~(0xf << (4 * i));
    PWM->TCFG1 |= (value << (4 * i));
}

void pwm_set_duration(unsigned int i, unsigned int value)
{
    (*PWM).CH[i].TCNTB = value;
}

void pwm_set_compare(unsigned int i, unsigned int value)
{
    (*PWM).CH[i].TCMPB = value;
}

unsigned int pwm_get_prescaler(unsigned int i)
{
    if (i < 2)
    {
        return PWM->TCFG0 & 0xff;
    }
    else
    {
        return (PWM->TCFG0 >> 8) & 0xff;

    }
}

unsigned int pwm_get_divider(unsigned int i)
{
    return (PWM->TCFG1 >> (4 * i)) & 0xf;
}

unsigned int pwm_get_duration(unsigned int i)
{
    return (*PWM).CH[i].TCNTB;
}

unsigned int pwm_get_compare(unsigned int i)
{
    return (*PWM).CH[i].TCMPB;
}

unsigned int pwm_get_count(unsigned int i)
{
    return (*PWM).CH[i].TCNTO;
}

void pwm_control(unsigned int i)
{
    int ofs = (i == 0) ? 0 : (4 * i + 4);

    PWM->TCON &= ~(0xf << ofs);

    PWM->TCON |= (0x1 << (ofs + 1)); //手动更新
    PWM->TCON |= (0x1 << (ofs + 3)); //自动装载
    PWM->TCON &= ~(0x1 << (ofs + 1)); //关闭手动更新
}

void pwm_start(unsigned int i)
{
    PWM->TCON |= (0x1 << ((i == 0) ? 0 : (4 * i + 4)));
}

void pwm_stop(unsigned int i)
{
    PWM->TCON &= ~(0x1 << ((i == 0) ? 0 : (4 * i + 4)));
    gpio_reset(i);
}

void pwm_config(unsigned int i, unsigned int pres, unsigned int div, unsigned int dur, unsigned int cmp)
{
    gpio_config(i);
    pwm_set_prescaler(i, pres);
    pwm_set_divider(i, div);
    pwm_set_duration(i, dur);
    pwm_set_compare(i, cmp);
    pwm_control(i);
    //pwm_start(i);
}

void pwm_virt_addr_init(void)
{
    if (ref_cnt == 0)
    {
        PWM = ioremap(PWM_BASE_ADDR, sizeof(struct PWM_REGS));
        GPIOC = ioremap(GPIOC_BASE_ADDR, sizeof(struct GPIO_REGS));
        GPIOD = ioremap(GPIOD_BASE_ADDR, sizeof(struct GPIO_REGS));
    }
    ref_cnt++;
}

void pwm_virt_addr_release(void)
{
    if (ref_cnt == 0) return;
    ref_cnt--;
    if (ref_cnt == 0)
    {
        iounmap(PWM);
        iounmap(GPIOC);
        iounmap(GPIOD);
    }
}

