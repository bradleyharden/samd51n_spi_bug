#include "samd51n20a.h" 

// Define PORT Groups for easier access
#define PORTA PORT->Group[0]
#define PORTB PORT->Group[1]
#define PORTC PORT->Group[2]

int main()
{
    // Source GCLK2 from DFLL and divide by 2 for a 24 MHz output
    GCLK->GENCTRL[2].reg = (
        GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DFLL) |
        GCLK_GENCTRL_DIV(2) |
        GCLK_GENCTRL_GENEN
    );
    while (GCLK->SYNCBUSY.bit.GENCTRL2);

    // Configure SERCOM2 to use GCLK2
    GCLK->PCHCTRL[23].bit.GEN = GCLK_PCHCTRL_GEN_GCLK2_Val;
    GCLK->PCHCTRL[23].bit.CHEN = 1;

    // Enable the APB clock for SERCOM2
    MCLK->APBBMASK.bit.SERCOM2_ = 1;

    // Configure the SCLK, MISO and MOSI pins for SERCOM2, IOSET4
    // Any SERCOM/IOSET pair should give the same behavior
    PORTB.PMUX[12].bit.PMUXE = MUX_PB24D_SERCOM2_PAD1;
    PORTB.PMUX[12].bit.PMUXO = MUX_PB25D_SERCOM2_PAD0;
    PORTC.PMUX[12].bit.PMUXO = MUX_PC25D_SERCOM2_PAD3;
    PORTB.PINCFG[24].bit.PMUXEN = 1;
    PORTB.PINCFG[25].bit.PMUXEN = 1;
    PORTC.PINCFG[25].bit.PMUXEN = 1;

    // Configure the SS pin as an output and bring it high.
    // This is unrelated to the strange behavior, but
    // I need to prove it to the tech support agent.
    PORTC.DIRSET.reg = (1 << 24);
    PORTC.PINCFG[24].reg = 0;
    PORTC.OUTSET.reg = (1 << 24);

    // Reset SERCOM2
    SERCOM2->SPI.CTRLA.bit.SWRST = 1;
    while (SERCOM2->SPI.SYNCBUSY.bit.SWRST || SERCOM2->SPI.CTRLA.bit.SWRST);

    // Configure SERCOM2 as an SPI master with MISO on pad 0 and MOSI on pad 3
    SERCOM2->SPI.CTRLA.reg = (
        SERCOM_SPI_CTRLA_DIPO(0) |
        SERCOM_SPI_CTRLA_DOPO(2) |
        SERCOM_SPI_CTRLA_MODE(3)
    );

    // Configure the BAUD rate for GCLK2 / 2 = 12 MHz
    SERCOM2->SPI.BAUD.reg = SERCOM_SPI_BAUD_BAUD(0);

    // Enable 32-bit extension mode
    SERCOM2->SPI.CTRLC.reg = SERCOM_SPI_CTRLC_DATA32B;

    // Configure the length counter for 5 byte transactions and enable it.
    // The bug occurs for legnths of 4 * N + 1, where N is a positive integer.
    // It does NOT occur for other lengths.
    SERCOM2->SPI.LENGTH.reg = SERCOM_SPI_LENGTH_LEN(5) | SERCOM_SPI_LENGTH_LENEN;
    while (SERCOM2->SPI.SYNCBUSY.bit.LENGTH);

    // Enable the receive buffer
    SERCOM2->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN; 
    while (SERCOM2->SPI.SYNCBUSY.bit.CTRLB);

    // Enable SERCOM2
    SERCOM2->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
    while (SERCOM2->SPI.SYNCBUSY.bit.ENABLE);

    // Bring SS low.
    // This has no effect on the strange behavior.
    PORTC.OUTCLR.reg = (1 << 24);

    // Conduct three transactions.
    // Each transaction *should* send five bytes of 0xAA.
    // However, the first transaction actually sends six bytes:
    // five 0xAA followed by one 0x00.
    // The next two transactions each send five bytes, as expected.
    for (unsigned i = 0; i < 3; i++) {
        const uint8_t LEN = 2;
        uint8_t sent = 0;
        uint8_t rcvd = 0;
        uint32_t _;
        while (rcvd < LEN) {
            SERCOM_SPI_INTFLAG_Type flags = SERCOM2->SPI.INTFLAG;
            if (sent < LEN && flags.bit.DRE) {
                SERCOM2->SPI.DATA.reg = (sent == 0) ? 0xAAAAAAAA : 0x000000AA;
                sent += 1;
            }
            if (rcvd < LEN && flags.bit.RXC) {
                _ = SERCOM2->SPI.DATA.reg;
                rcvd += 1;
            }
        }
    }

    // Bring SS high again
    PORTC.OUTSET.reg = (1 << 24);

    // End
    while (1);
}