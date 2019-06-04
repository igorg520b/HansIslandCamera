bool sleeping = false;

void initializeWDT() {
        // One-time initialization of watchdog timer.
    // Generic clock generator 2, divisor = 32 (2^(DIV+1))
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
    // Enable clock generator 2 using low-power 32KHz oscillator.
    // With /32 divisor above, this yields 1024Hz(ish) clock.
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) |
                        GCLK_GENCTRL_GENEN |
                        GCLK_GENCTRL_SRC_OSCULP32K |
                        GCLK_GENCTRL_DIVSEL;
    while(GCLK->STATUS.bit.SYNCBUSY);
    // WDT clock = clock gen 2
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT |
                        GCLK_CLKCTRL_CLKEN |
                        GCLK_CLKCTRL_GEN_GCLK2;

    // Enable WDT early-warning interrupt
    NVIC_DisableIRQ(WDT_IRQn);
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0); // Top priority
    NVIC_EnableIRQ(WDT_IRQn);
}

int enableWatchdog() {
  WDT->CTRL.reg = 0; // Disable watchdog for config
  while(WDT->STATUS.bit.SYNCBUSY);

  WDT->INTENSET.bit.EW   = 1;      // Enable early warning interrupt
  WDT->CONFIG.bit.PER    = 0xB;    // Period = max
  WDT->EWCTRL.bit.EWOFFSET = 0xA;
  while(WDT->STATUS.bit.SYNCBUSY); // Sync CTRL write

  resetWD();
  WDT->CTRL.bit.ENABLE = 1;            // Start watchdog now!
  while(WDT->STATUS.bit.SYNCBUSY);
}

void resetWD() {
    // Write the watchdog clear key value (0xA5) to the watchdog
    // clear register to clear the watchdog timer and reset it.
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    while(WDT->STATUS.bit.SYNCBUSY);
}

void sleepWell() {
  resetWD();
  // WDT must be enabled earlier
  // Enable standby sleep mode (deepest sleep) and activate.
  // Don't fully power down flash when in sleep
  NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  sleeping = true;
  __DSB(); // Data sync to ensure outgoing memory accesses complete
  __WFI(); // Wait for interrupt (places device in sleep mode)
  sleeping = false;
  // Code resumes here on wake (WDT early warning interrupt).
}

void WDT_Handler(void) {
  if(sleeping) resetWD();
  WDT->INTFLAG.bit.EW  = 1;        // Clear interrupt flag
}
