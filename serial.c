#include <stdint.h>
#include "serial.h"
#include "memio.h"

#define SERIAL_USB

#ifdef SERIAL_UART

#define UART_IS_DLL 0x100
#define UART_IS_LCR 0x200

#define UART_RBR 0x00
#define UART_THR 0x00
#define UART_IER 0x04
#define UART_IIR 0x08
#define UART_FCR 0x08
#define UART_LCR 0x0c
#define UART_MCR 0x10
#define UART_LSR 0x14
#define UART_MSR 0x18
#define UART_SCR 0x1c

#define UART_SPEED 0x24

/* The following are active when LCR[7] = 1 */
#define UART_DLL 0x100
#define UART_DLH 0x104

/* The following are active when LCR = 0xbf */
#define UART_EFR   0x208
#define UART_XON1  0x210
#define UART_XON2  0x214
#define UART_XOFF1 0x218
#define UART_XOFF2 0x21c

#define UART_BASE 0xa0080000

enum uart_baudrate {
	UART_38400,
	UART_57600,
	UART_115200,
	UART_230400,
	UART_460800,
	UART_614400,
	UART_921600,
};
#define UART_BAUD_RATE UART_115200

#if 0
/* 26MHz clock input (used when no PLL initialized directly after poweron) */
static const uint16_t divider[] = {
	[UART_38400]    = 42,
	[UART_57600]    = 28,
	[UART_115200]   = 14,
	[UART_230400]   = 7,
	[UART_460800]   = 14,   /* would need UART_REG(HIGHSPEED) = 1 or 2 */
	[UART_921600]   = 7,    /* would need UART_REG(HIGHSPEED) = 2 */
};
#else
/* 52MHz clock input (after PLL init) */
static const uint16_t divider[] = {
	[UART_38400]    = 85,
	[UART_57600]    = 56,
	[UART_115200]   = 28,
	[UART_230400]   = 14,
	[UART_460800]   = 7,
	[UART_921600]   = 7,    /* would need UART_REG(HIGHSPEED) = 1 */
};
#endif

static uint8_t uart_getreg(int regnum)
{
	volatile uint32_t *reg = (uint32_t *)(UART_BASE + (regnum & 0xff));
	return *reg;
}

static void uart_setreg(int regnum, uint8_t val)
{
	uint8_t old_lcr;
	if (regnum & UART_IS_DLL)
		uart_setreg(UART_LCR, uart_getreg(UART_LCR) | 0x80);
	else if (regnum & UART_IS_LCR) {
		old_lcr = uart_getreg(UART_LCR);
		uart_setreg(UART_LCR, 0xbf);
	}

	volatile uint32_t *reg = (uint32_t *)(UART_BASE + (regnum & 0xff));
	*reg = val;

	if (regnum & UART_IS_DLL)
		uart_setreg(UART_LCR, uart_getreg(UART_LCR) & ~0x80);
	else if (regnum & UART_IS_LCR)
		uart_setreg(UART_LCR, old_lcr);
}

int serial_putc(uint8_t c)
{
	/* Wait for UART to be empty */
	while (! (uart_getreg(UART_LSR) & 0x20))
		asm("");

	uart_setreg(UART_RBR, c);
	return 0;
}

uint8_t serial_getc(void)
{
	while (! (uart_getreg(UART_LSR) & 0x01))
		asm("");
	return uart_getreg(UART_RBR);
}

int serial_puts(const void *s)
{
	const char *str = s;
	while(*str) {
		if (*str == '\n')
			serial_putc('\r');
		serial_putc(*str++);
	}
	return 0;
}

int serial_read(void *data, int bytes)
{
	while (--bytes)
		((uint8_t *)data++) = serial_getc();
}

void serial_init(void)
{
	int tmp;

	// Setup 8-N-1,(UART_WLS_8 | UART_NONE_PARITY | UART_1_STOP) = 0x03
	uart_setreg(UART_LCR, 0x03);

	// Set BaudRate 
	// config by UART_BAUD_RATE(9:115200)
	uart_setreg(UART_DLL, divider[UART_BAUD_RATE]&0xff);
	uart_setreg(UART_DLH, divider[UART_BAUD_RATE]>>8);
	uart_setreg(UART_LCR, 0x03);

	// Enable Fifo, and Rx Trigger level = 16bytes, flush Tx, Rx fifo
	// (UART_FCR_FIFOEN | UART_FCR_4Byte_Level | UART_FCR_RFR | UART_FCR_TFR) = 0x47
	uart_setreg(UART_FCR, 0x47);

	// DTR , RTS is on, data will be coming,Output2 is high
	uart_setreg(UART_MCR, 0x03);

	/* Set up normal interrupts */
	uart_setreg(UART_IER, 0x0d);

	/* Pause a while */
	for (tmp=0; tmp<0xff; tmp++);
}
#else /* SERIAL_USB */

#include "fernvale-usb.h"

static volatile uint8_t *recv_bfr = (uint8_t *)0x70000000;
static int recv_size = 0;
static int recv_offset = 0;
static int send_max = 0;
static int send_cur = 0;

#define USB_MODE_OUT 0
#define USB_MODE_IN 1

static void usb_set_mode(uint8_t epnum, int in)
{
	writeb(epnum, USB_CTRL_INDEX);

	if (in) {
		if (readb(USB_CTRL_EP_INCSR2) & USB_CTRL_EP_INCSR2_MODE)
			/* Already set to "IN" */
			return;

		writeb(readb(USB_CTRL_EP_INCSR2) | USB_CTRL_EP_INCSR2_MODE,
			USB_CTRL_EP_INCSR2);
	}
	else {
		if (! (readb(USB_CTRL_EP_INCSR2) & USB_CTRL_EP_INCSR2_MODE))
			/* Already set to "OUT" */
			return;

		writeb(readb(USB_CTRL_EP_INCSR2) & ~USB_CTRL_EP_INCSR2_MODE,
			USB_CTRL_EP_INCSR2);
	}
}

static void usb_flush_output(int epnum)
{
	/* Set endpoint to IN */
	usb_set_mode(epnum, USB_MODE_IN);

	/* Begin transmitting the packet */
	writeb(USB_CTRL_EP_INCSR1_INPKTRDY, USB_CTRL_EP_INCSR1);

	/* Wait for the character to transmit, so we don't double-xmit */
	while (!readb(USB_CTRL_INTRIN))
		asm("");

	/* Set endpoint back to OUT */
	usb_set_mode(epnum, USB_MODE_OUT);

	send_cur = 0;
}

static void usb_receive_wait(uint8_t epnum)
{
	uint32_t fifo_register = USB_CTRL_EP0_FIFO_DB0 + (epnum * 4);

	/* Wait for data to exist, ignoring other USB IRQs */
	while (!readb(USB_CTRL_INTROUT))
		(void)readb(USB_CTRL_INTRUSB);

	/* Select EP1 */
	writeb(epnum, USB_CTRL_INDEX);

	while (!(readb(USB_CTRL_EP_OUTCSR1) & USB_CTRL_EP_OUTCSR1_RXPKTRDY))
		asm("");

	recv_size  = (readb(USB_CTRL_EP_COUNT1) << 0) & 0x00ff;
	recv_size |= (readb(USB_CTRL_EP_COUNT2) << 8) & 0x0300;
	recv_offset = 0;

	int bytes_to_read = recv_size + 1;
	int off = 0;

	/* Fill in the receive buffer */
	while (bytes_to_read) {
		if (bytes_to_read >= 4) {
			*((uint32_t *)(recv_bfr + off)) = readl(fifo_register);
			bytes_to_read -= 4;
			off += 4;
		}
		else if (bytes_to_read >= 2) {
			*((uint16_t *)(recv_bfr + off)) = readw(fifo_register);
			bytes_to_read -= 2;
			off += 2;
		}
		else {
			*((uint8_t *)(recv_bfr + off)) = readb(fifo_register);
			bytes_to_read -= 1;
			off += 1;
		}
	}

	/* Clear FIFO (write 0 to RXPKTRDY) */
	writeb(0, USB_CTRL_EP_OUTCSR1);
}

static void usb_handle_irqs(int epnum)
{
	/* Ignore general-purpose IRQs */
	(void)readb(USB_CTRL_INTRUSB);

	/* Select EP1 */
	writeb(epnum, USB_CTRL_INDEX);

	/* If data exists in the output FIFOs, send the packet */
	if (send_cur)
		usb_flush_output(epnum);

	/* If there are incoming bytes, read them into the buffer */
	if (readb(USB_CTRL_EP_OUTCSR1) & USB_CTRL_EP_OUTCSR1_RXPKTRDY)
		usb_receive_wait(epnum);
}

int serial_putc(uint8_t c)
{
	/* Add the character to the FIFO */
	writeb(c, USB_CTRL_EP1_FIFO_DB0);
	send_cur++;

	if (send_cur >= send_max)
		usb_flush_output(1);

	return 0;
}

uint8_t serial_getc(void)
{
	/* Wait for data if the buffer is empty */
	while (!recv_size)
		usb_handle_irqs(1);

	recv_size--;
	return recv_bfr[recv_offset++];
}

int serial_puts(const void *s)
{
	const char *str = s;
	while(*str) {
		/* Fix up linefeeds */
		if (*str == '\n')
			serial_putc('\r');

		serial_putc(*str++);
	}
	return 0;
}

void serial_write(const void *d, int bytes)
{
	const char *str = d;
	int i;

	for (i = 0; i < bytes; i++)
		serial_putc(str[i]);
}

int serial_read(void *data, int bytes)
{
	int i;
	for (i = 0; i < bytes; i++) {
		*((uint8_t *)data) = serial_getc();
		data++;
	}

	return 0;
}

void serial_init(void)
{
	send_max = 32;
	send_cur = 0;

	writel(readl(USB_CTRL_CON) | USB_CTRL_CON_NULLPKT_FIX, USB_CTRL_CON);

	(void)readb(USB_CTRL_INTROUT);
	(void)readb(USB_CTRL_INTRIN);
	(void)readb(USB_CTRL_INTRUSB);

	writeb(0, USB_CTRL_INTROUTE);
	(void)readb(USB_CTRL_INTROUTE);

	writeb(0, USB_CTRL_INTRINE);
	(void)readb(USB_CTRL_INTRINE);

	writeb(USB_CTRL_INTROUTE_EP1_OUT_ENABLE, USB_CTRL_INTROUTE);
	writeb(USB_CTRL_INTRINE_EP1_IN_ENABLE | USB_CTRL_INTRINE_EP0_ENABLE,
			USB_CTRL_INTRINE);

	/* Select EP1 */
	writeb(1, USB_CTRL_INDEX);

	/* Make sure the packet size is 64 bytes */
	writeb(send_max, USB_CTRL_EP_INMAXP);

	/* Clear FIFO */
	writeb(USB_CTRL_EP_INCSR1_FLUSHFIFO, USB_CTRL_EP_INCSR1);

	/* Clear the second packet */
	writeb(USB_CTRL_EP_INCSR1_FLUSHFIFO, USB_CTRL_EP_INCSR1);

	/* Set up FIFO to automatically transmit when the buffer is full */
	//writeb(USB_CTRL_EP_INCSR2_AUTOSET, USB_CTRL_EP_INCSR2);
	writeb(0, USB_CTRL_EP_INCSR2);

	/* Set the USB mode to OUT, to ensure we receive packets from host */
	usb_set_mode(1, USB_MODE_OUT);

	recv_offset = 0;
	recv_size = 0;
	usb_handle_irqs(1);
}

#endif /* !SERIAL_UART */
