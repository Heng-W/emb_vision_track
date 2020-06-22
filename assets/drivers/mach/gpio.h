#ifndef S5P4418_GPIO_H
#define S5P4418_GPIO_H


#define GPIOA_BASE_ADDR  (0xC001A000)
#define GPIOB_BASE_ADDR  (0xC001B000)
#define GPIOC_BASE_ADDR  (0xC001C000)
#define GPIOD_BASE_ADDR  (0xC001D000)
#define GPIOE_BASE_ADDR  (0xC001E000)


struct GPIO_REGS
{
    unsigned int OUT;
    unsigned int OUTENB;//0:input  1:output
    unsigned int RESERVED[6];
    unsigned int ALTFN0;//0-15pin
    unsigned int ALTFN1;//16-31pin
};



#define gpio_dir_out(port,pin)    (port)->OUTENB |= (1 << (pin))
#define gpio_dir_in(port,pin)    (port)->OUTENB &= ~(1 << (pin))

#define gpio_set_high(port,pin)  (port)->OUT |= (1 << (pin))
#define gpio_set_low(port,pin)  (port)->OUT &= ~(1 << (pin))

#define gpio_read(port,pin)  ((((port)->OUT)>>(pin)) & 0x1)


void gpio_set_func(volatile struct GPIO_REGS* port, unsigned int pin, unsigned int fn);


#endif //S5P4418_GPIO_H
