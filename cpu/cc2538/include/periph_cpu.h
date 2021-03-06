/*
 * Copyright (C) 2015-2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         cpu_cc2538
 * @{
 *
 * @file
 * @brief           CPU specific definitions for internal peripheral handling
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

#include <stdint.h>

#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Starting offset of CPU_ID
 */
#define CPUID_ADDR          (&IEEE_ADDR_MSWORD)
/**
 * @brief   Length of the CPU_ID in octets
 */
#define CPUID_LEN           (8U)

/**
 * @brief   Define a custom type for GPIO pins
 * @{
 */
#define HAVE_GPIO_T
typedef uint32_t gpio_t;
/** @} */

/**
 * @brief   Define a custom GPIO_PIN macro
 *
 * For the CC2538, we use OR the gpio ports base register address with the
 * actual pin number.
 */
#define GPIO_PIN(port, pin) (gpio_t)(((uint32_t)GPIO_A + (port << 12)) | pin)

/**
 * @brief   Define a custom GPIO_UNDEF value
 */
#define GPIO_UNDEF 99

/**
 * @brief   I2C configuration options
 */
typedef struct {
    gpio_t scl_pin;         /**< pin used for SCL */
    gpio_t sda_pin;         /**< pin used for SDA */
} i2c_conf_t;

/**
 * @brief declare needed generic SPI functions
 * @{
 */
#define PERIPH_SPI_NEEDS_INIT_CS
#define PERIPH_SPI_NEEDS_TRANSFER_BYTE
#define PERIPH_SPI_NEEDS_TRANSFER_REG
#define PERIPH_SPI_NEEDS_TRANSFER_REGS
/** @} */

/**
 * @brief   Override the default GPIO mode settings
 * @{
 */
#define HAVE_GPIO_MODE_T
typedef enum {
    GPIO_IN    = ((uint8_t)0x00),               /**< input, no pull */
    GPIO_IN_PD = ((uint8_t)IOC_OVERRIDE_PDE),   /**< input, pull-down */
    GPIO_IN_PU = ((uint8_t)IOC_OVERRIDE_PUE),   /**< input, pull-up */
    GPIO_OUT   = ((uint8_t)IOC_OVERRIDE_OE),    /**< output */
    GPIO_OD    = (0xff),                        /**< not supported */
    GPIO_OD_PU = (0xff)                         /**< not supported */
} gpio_mode_t;
/** @} */

/**
 * @brief   Override SPI mode settings
 * @{
 */
#define HAVE_SPI_MODE_T
typedef enum {
    SPI_MODE_0 = 0,                             /**< CPOL=0, CPHA=0 */
    SPI_MODE_1 = (SSI_CR0_SPH),                 /**< CPOL=0, CPHA=1 */
    SPI_MODE_2 = (SSI_CR0_SPO),                 /**< CPOL=1, CPHA=0 */
    SPI_MODE_3 = (SSI_CR0_SPO | SSI_CR0_SPH)    /**< CPOL=1, CPHA=1 */
} spi_mode_t;
/** @ */

/**
 * @brief   Override SPI clock settings
 * @{
 */
#define HAVE_SPI_CLK_T
typedef enum {
    SPI_CLK_100KHZ = 0,     /**< drive the SPI bus with 100KHz */
    SPI_CLK_400KHZ = 1,     /**< drive the SPI bus with 400KHz */
    SPI_CLK_1MHZ   = 2,     /**< drive the SPI bus with 1MHz */
    SPI_CLK_5MHZ   = 3,     /**< drive the SPI bus with 5MHz */
    SPI_CLK_10MHZ  = 4      /**< drive the SPI bus with 10MHz */
} spi_clk_t;
/** @} */

/**
 * @brief   Datafields for static SPI clock configuration values
 */
typedef struct {
    uint8_t cpsr;           /**< CPSR clock divider */
    uint8_t scr;            /**< SCR clock divider */
} spi_clk_conf_t;

/**
 * @brief   SPI configuration data structure
 * @{
 */
typedef struct {
    cc2538_ssi_t *dev;      /**< SSI device */
    gpio_t mosi_pin;        /**< pin used for MOSI */
    gpio_t miso_pin;        /**< pin used for MISO */
    gpio_t sck_pin;         /**< pin used for SCK */
    gpio_t cs_pin;          /**< pin used for CS */
} spi_conf_t;
/** @} */

/**
 * @brief   Timer configuration data
 */
typedef struct {
    cc2538_gptimer_t *dev;  /**< timer device */
    uint_fast8_t channels;  /**< number of channels */
    uint_fast8_t cfg;       /**< timer config word */
} timer_conf_t;

/**
 * @brief   Override resolution options
 */
#define HAVE_ADC_RES_T
typedef enum {
    ADC_RES_6BIT  = 0xa00,          /**< not supported by hardware */
    ADC_RES_7BIT  = (0 << 4),       /**< ADC resolution: 7 bit */
    ADC_RES_8BIT  = 0xb00,          /**< not supported by hardware */
    ADC_RES_9BIT  = (1 << 4),       /**< ADC resolution: 9 bit */
    ADC_RES_10BIT = (2 << 4),       /**< ADC resolution: 10 bit */
    ADC_RES_12BIT = (3 << 4),       /**< ADC resolution: 12 bit */
    ADC_RES_14BIT = 0xc00,          /**< not supported by hardware */
    ADC_RES_16BIT = 0xd00,          /**< not supported by hardware */
} adc_res_t;

/**
 * @brief Masks for getting data
 * @{
 */
#define SOCADC_7_BIT_RSHIFT        9 // Mask for getting data( 7 bits ENOB)
#define SOCADC_9_BIT_RSHIFT        7 // Mask for getting data( 9 bits ENOB)
#define SOCADC_10_BIT_RSHIFT       6 // Mask for getting data(10 bits ENOB)
#define SOCADC_12_BIT_RSHIFT       4 // Mask for getting data(12 bits ENOB)
/** @} */

#ifdef __cplusplus
}
#endif

#include "periph/dev_enums.h"

#endif /* PERIPH_CPU_H */
/** @} */
