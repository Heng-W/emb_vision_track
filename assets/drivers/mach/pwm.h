#ifndef S5P4418_PWM_H
#define S5P4418_PWM_H



void gpio_reset(unsigned int i);
void pwm_set_prescaler(unsigned int i, unsigned int value);
void pwm_set_divider(unsigned int i, unsigned int value);
void pwm_set_duration(unsigned int i, unsigned int value);
void pwm_set_compare(unsigned int i, unsigned int value);
unsigned int pwm_get_prescaler(unsigned int i);
unsigned int pwm_get_divider(unsigned int i);
unsigned int pwm_get_duration(unsigned int i);
unsigned int pwm_get_compare(unsigned int i);
unsigned int pwm_get_count(unsigned int i);
void pwm_control(unsigned int i);
void pwm_start(unsigned int i);
void pwm_stop(unsigned int i);
void pwm_config(unsigned int i, unsigned int pres, unsigned int div, unsigned int dur, unsigned int cmp);
void pwm_virt_addr_init(void);
void pwm_virt_addr_release(void);



#endif //S5P4418_PWM_H
