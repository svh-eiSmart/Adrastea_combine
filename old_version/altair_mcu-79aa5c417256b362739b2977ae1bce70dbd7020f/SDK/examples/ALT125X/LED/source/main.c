#include "DRV_LED.h"
#include "mini_console.h"

int main(void) {
  DRV_LED_Initialize();

  mini_console();

  return 0;
}
