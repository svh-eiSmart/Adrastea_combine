#include "125X_mcu.h"

#define CLK_15MHZ 15000000UL
#define CLK_20MHZ 20000000UL
#define CLK_30MHZ 30000000UL
#define CLK_40MHZ 40000000UL
#define CLK_60MHZ 60000000UL
#define CLK_80MHZ 80000000UL
#define CLK_20MHZ_VCO 260000000UL
#define CLK_40MHZ_VCO 520000000UL
#define CLK_60MHZ_VCO 780000000UL
#define CLK_80MHZ_VCO 1040000000UL
#define RCC_15MHZ_DIV 7
#define RCC_30MHZ_DIV 3
#define RCC_60MHZ_DIV 1

typedef enum {
  /*  Following have to be init with RCC*/
  RCC_60mhz = 0,
  RCC_30mhz = 1,
  RCC_15mhz = 2,
  /* Following have to be init with PLL*/
  PLL_20mhz = 3,
  PLL_40mhz = 4,
  PLL_60mhz = 5,
  PLL_80mhz = 6
} Clock_rate_e;

extern uint32_t SystemCoreClock;

extern void rcclk_init(char clk_div);
extern void pll_init(ulong vco_freq, ulong clk_freq);

void sysclk_init() {
  Clock_rate_e clk_sel = RCC_60mhz;

  switch (clk_sel) {
    case RCC_15mhz:
      SystemCoreClock = CLK_15MHZ;
      rcclk_init(RCC_15MHZ_DIV);
      break;

    case RCC_30mhz:
      SystemCoreClock = CLK_30MHZ;
      rcclk_init(RCC_30MHZ_DIV);
      break;

    case RCC_60mhz:
      SystemCoreClock = CLK_60MHZ;
      rcclk_init(RCC_60MHZ_DIV);
      break;

    case PLL_20mhz:
      SystemCoreClock = CLK_20MHZ;
      pll_init(CLK_20MHZ_VCO, CLK_20MHZ);
      break;

    case PLL_40mhz:
      SystemCoreClock = CLK_40MHZ;
      pll_init(CLK_40MHZ_VCO, CLK_40MHZ);
      break;

    case PLL_60mhz:
      SystemCoreClock = CLK_60MHZ;
      pll_init(CLK_60MHZ_VCO, CLK_60MHZ);
      break;

    case PLL_80mhz:
      SystemCoreClock = CLK_80MHZ;
      pll_init(CLK_80MHZ_VCO, CLK_80MHZ);
      break;

    default:
      SystemCoreClock = CLK_60MHZ;
      rcclk_init(RCC_60MHZ_DIV);
      break;
  }
}
