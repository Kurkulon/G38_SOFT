#include "types.h"
#include "core.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define PIN_EN		4
#define PIN_RTS		27
#define PIN_ISP		12
#define PIN_HIN1	17
#define PIN_UTX		16
#define PIN_URX		26
#define PIN_PULSE	15

#define EN		(1<<PIN_EN)
#define RTS		(1<<PIN_RTS)
#define ISP		(1<<PIN_ISP)
#define HIN1	(1<<PIN_HIN1)

/*----------------------------------------------------------------------------
  Initialize the system
 *----------------------------------------------------------------------------*/
extern "C" void SystemInit()
{
	u32 i;
	using namespace CM0;
	using namespace HW;

	SYSCON->SYSAHBCLKCTRL |= CLK::SWM_M | CLK::IOCON_M | CLK::GPIO_M | HW::CLK::MRT_M | HW::CLK::UART0_M | HW::CLK::CRC_M | HW::CLK::DMA_M;

	GPIO->DIRSET0 = RTS|ISP|EN|HIN1;
	GPIO->CLR0 = RTS|EN|HIN1;
	GPIO->SET0 = 0;

	IOCON->PIO0_1.B.MODE = 0;

	HW::GPIO->NOT0 = ISP;

	SWM->PINENABLE0.B.CLKIN = 0;

	for (i = 0; i < 200; i++) __nop();

	SYSCON->SYSPLLCLKSEL  = 3;					/* Select PLL Input         */
	SYSCON->SYSPLLCLKUEN  = 0;					/* Update Clock Source      */
	SYSCON->SYSPLLCLKUEN  = 1;					/* Update Clock Source      */
	while (!(SYSCON->SYSPLLCLKUEN & 1));		/* Wait Until Updated       */

	HW::GPIO->NOT0 = ISP;

	SYSCON->MAINCLKSEL    = 1;					/* Select PLL Clock Output  */
	SYSCON->MAINCLKUEN    = 0;					/* Update MCLK Clock Source */
	SYSCON->MAINCLKUEN    = 1;					/* Update MCLK Clock Source */
	while (!(SYSCON->MAINCLKUEN & 1));			/* Wait Until Updated       */

	HW::GPIO->NOT0 = ISP;

//	SYSCON->SYSAHBCLKDIV  = SYSAHBCLKDIV_Val;

	SYSCON->UARTCLKDIV = 1;
	SWM->U0_RXD = PIN_URX;
	SWM->U0_TXD = PIN_UTX;

	DMA->SRAMBASE = DmaTable;
	DMA->CTRL = 1;

	HW::GPIO->NOT0 = ISP;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
