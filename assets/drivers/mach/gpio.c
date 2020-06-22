#include "gpio.h"

void gpio_set_func(volatile struct GPIO_REGS* port, unsigned int pin, unsigned int fn)
{
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
