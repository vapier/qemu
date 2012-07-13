/*
 * Blackfin Universal Asynchronous Receiver/Transmitter (UART) model.
 * For "old style" UARTs on BF53x/etc... parts.
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

#ifndef DV_BFIN_UART_H
#define DV_BFIN_UART_H

/* UART_LCR */
#define DLAB         (1 << 7)

/* UART_LSR */
#define TFI          (1 << 7)
#define TEMT         (1 << 6)
#define THRE         (1 << 5)
#define BI           (1 << 4)
#define FE           (1 << 3)
#define PE           (1 << 2)
#define OE           (1 << 1)
#define DR           (1 << 0)

/* UART_IER */
#define ERBFI        (1 << 0)
#define ETBEI        (1 << 1)
#define ELSI         (1 << 2)

/* UART_MCR */
#define XOFF         (1 << 0)
#define MRTS         (1 << 1)
#define RFIT         (1 << 2)
#define RFRT         (1 << 3)
#define LOOP_ENA     (1 << 4)
#define FCPOL        (1 << 5)
#define ARTS         (1 << 6)
#define ACTS         (1 << 7)

#endif
