#ifndef HW_CONF_H__25_11_2022__16_53
#define HW_CONF_H__25_11_2022__16_53

#include <types.h>
#include <core.h>

#define MCK_MHz 25
#define MCK (MCK_MHz*1000000)
#define NS2CLK(x) (((x)*MCK_MHz+500)/1000)
#define US2CLK(x) ((x)*MCK_MHz)
#define MS2CLK(x) ((x)*MCK_MHz*1000)

#define BOOTLOADER

// ++++++++++++++	USIC	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define USART0_USIC_NUM		0
//#define USART1_USIC_NUM	1
//#define USART2_USIC_NUM	2
//#define SPI0_USIC_NUM		3
//#define SPI1_USIC_NUM		4
//#define I2C0_USIC_NUM		5
//#define I2C1_USIC_NUM		6
//#define I2C2_USIC_NUM		7
//#define I2C3_USIC_NUM		8

// ++++++++++++++	USART	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define PIN_UTX0			16 
#define PIN_URX0			26 
#define PIN_RTS0			27

#define UTX0				(1UL<<PIN_UTX0)
#define URX0				(1UL<<PIN_URX0)
#define RTS0				(1UL<<PIN_RTS0)

// ++++++++++++++	MOTOR	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define PIN_DHU				8 
#define PIN_DHV				9 
#define PIN_DHW				10
#define PIN_EN				4
#define PIN_HIN1			17

#define DHU					(1UL<<PIN_DHU)
#define DHV					(1UL<<PIN_DHV)
#define DHW					(1UL<<PIN_DHW)
#define HIN1				(1UL<<PIN_HIN1)
#define EN					(1UL<<PIN_EN)

#define PIN_ISP				12

#define ISP					(1UL<<PIN_ISP)


// ++++++++++++++	PIO INIT	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

																				
#define GPIO_INIT_DIR0		RTS0|EN|HIN1|ISP									
#define GPIO_INIT_PIN0		EN						


#endif // HW_CONF_H__25_11_2022__16_53
