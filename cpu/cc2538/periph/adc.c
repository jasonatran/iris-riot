#include <stdio.h>
#include "board.h"
#include "cpu.h"
#include "sched.h"
#include "thread.h"
#include "periph_conf.h"
#include "cpu_conf.h"
#include "periph/adc.h"
#include "cc2538.h"
#include <debug.h>

enum
{
    EOC = 1,
    STSEL = 3,
};

int adc_init(adc_t line)
{
    /* make sure the given ADC line is valid */
    if (line >= ADC_NUMOF) {
        return -1;
    }

    cc2538_soc_adc_t *adcaccess;
    adcaccess = SOC_ADC;

    /* Setting up ADC */
    gpio_hardware_control(GPIO_PXX_TO_NUM(PORT_A, line));
    IOC_PXX_OVER[GPIO_PXX_TO_NUM(PORT_A, line)] = IOC_OVERRIDE_ANA;
    
    /* Start conversions when ADCCON1.ST = 1 */
    adcaccess->cc2538_adc_adccon1.ADCCON1 |= adcaccess->cc2538_adc_adccon1.ADCCON1bits.STSEL;

    IOC_PXX_OVER[GPIO_PXX_TO_NUM(PORT_A, line)] = IOC_OVERRIDE_ANA;

    return 0;
}

int adc_sample(adc_t line, adc_res_t res)
{
    /* make sure the given ADC line is valid */
    if (line >= ADC_NUMOF) {
        return -1;
    }

    cc2538_soc_adc_t *adcaccess = SOC_ADC;
    uint8_t refvoltage = SOC_ADC_ADCCON_REF_AVDD5; /* TODO: is hardcoded ref voltage OK? */
    int16_t result;

    /* Start a single extra conversion with the given parameters. */
    adcaccess->ADCCON3 = ((adcaccess->ADCCON3) & ~(SOC_ADC_ADCCON3_EREF | SOC_ADC_ADCCON3_EDIV | SOC_ADC_ADCCON3_ECH)) |
                         refvoltage | res | line;

    /* Poll until end of conversion */
    while ((adcaccess->cc2538_adc_adccon1.ADCCON1 & adcaccess->cc2538_adc_adccon1.ADCCON1bits.EOC) == 0);

    result  = (((adcaccess->ADCL) & 0xfc));
    result |= (((adcaccess->ADCH) & 0xff) << 8);
    switch (res)
    {
        case ADC_RES_7BIT:
            result = result >> SOCADC_7_BIT_RSHIFT;
            break;
        case ADC_RES_9BIT:
            result = result >> SOCADC_9_BIT_RSHIFT;
            break;
        case ADC_RES_10BIT:
            result = result >> SOCADC_10_BIT_RSHIFT;
            break;
        case ADC_RES_12BIT:
            result = result >> SOCADC_12_BIT_RSHIFT;
            break;
        default:
            return -1;
    }

    /* Return conversion result */
    return result;
}