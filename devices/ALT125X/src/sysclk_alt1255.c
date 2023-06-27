#include "125X_mcu.h"

#define CLK_50MHZ 50000000UL
#define RCC_50MHZ_DIV 1

extern uint32_t SystemCoreClock;

extern void rcclk_init(char clk_div);
extern void pll_init(ulong vco_freq, ulong clk_freq);

void sysclk_init() {
  SystemCoreClock = CLK_50MHZ;
  rcclk_init(RCC_50MHZ_DIV);
}
