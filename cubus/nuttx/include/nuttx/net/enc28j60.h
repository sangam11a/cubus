/****************************************************************************
 * include/nuttx/net/enc28j60.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_NET_ENC28J60_H
#define __INCLUDE_NUTTX_NET_ENC28J60_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ENC28J60 Configuration Settings:
 *
 * CONFIG_ENC28J60 - Enabled ENC28J60 support
 * CONFIG_ENC28J60_SPIMODE - Controls the SPI mode
 * CONFIG_ENC28J60_FREQUENCY - Define to use a different bus frequency
 * CONFIG_ENC28J60_NINTERFACES - Specifies the number of physical ENC28J60
 *   devices that will be supported.
 * CONFIG_ENC28J60_HALFDUPPLEX - Default is full duplex
 */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The ENC28J60 normal provides interrupts to the MCU via a GPIO pin.  The
 * following structure provides an MCU-independent mechanixm for controlling
 * the ENC28J60 GPIO interrupt.
 *
 * The ENC32J60 interrupt is an active low, *level* interrupt. "When an
 * interrupt occurs, the interrupt flag is set. If the interrupt is enabled
 * in the EIE register and the INTIE global interrupt enable bit is set, the
 * INT pin will be driven low"
 *
 * "When an enabled interrupt occurs, the interrupt pin will remain low until
 * all flags which are causing the interrupt are cleared or masked off
 * (enable bit is cleared) by the host controller."  However, the interrupt
 * will behave like a falling edge interrupt because "After an interrupt
 * occurs, the host controller [clears] the global enable bit for the
 * interrupt pin before servicing the interrupt. Clearing the enable bit
 * will cause the interrupt pin to return to the non-asserted state (high).
 * Doing so will prevent the host controller from missing a falling edge
 * should another interrupt occur while the immediate interrupt is being
 * serviced."
 */

struct enc_lower_s
{
  int  (*attach)(FAR const struct enc_lower_s *lower, xcpt_t handler,
                 FAR void *arg);
  void (*enable)(FAR const struct enc_lower_s *lower);
  void (*disable)(FAR const struct enc_lower_s *lower);
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: enc_initialize
 *
 * Description:
 *   Initialize the Ethernet driver.  The ENC28J60 device is assumed to be
 *   in the post-reset state upon entry to this function.
 *
 * Input Parameters:
 *   spi   - A reference to the platform's SPI driver for the ENC28J60
 *   lower - The MCU-specific interrupt used to control low-level MCU
 *           functions (i.e., ENC28J60 GPIO interrupts).
 *   devno - If more than one ENC28J60 is supported, then this is the
 *           zero based number that identifies the ENC28J60;
 *
 * Returned Value:
 *   OK on success; Negated errno on failure.
 *
 * Assumptions:
 *
 ****************************************************************************/

struct spi_dev_s; /* see nuttx/spi/spi.h */
int enc_initialize(FAR struct spi_dev_s *spi,
                   FAR const struct enc_lower_s *lower, unsigned int devno);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_NET_ENC28J60_H */
