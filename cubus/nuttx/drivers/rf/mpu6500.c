/****************************************************************************
 * drivers/rf/dat-31r5-sp.c
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* Character driver for the Mini-Circuits MPU6500 digital step
 * attenuator.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/fs.h>
#include <nuttx/spi/spi.h>
#include <nuttx/rf/ioctl.h>
#include <nuttx/rf/attenuator.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_SPI) && defined(CONFIG_RF_MPU6500)

#ifndef CONFIG_MPU6500_SPI_FREQUENCY
#  define CONFIG_MPU6500_SPI_FREQUENCY 1000000
#endif

#define MPU6500_SPI_MODE (SPIDEV_MODE0) /* SPI Mode 0: CPOL=0,CPHA=0 */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mpu6500_dev_s
{
  FAR struct spi_dev_s *spi;    /* Saved SPI driver instance */
  int spidev;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Character driver methods */

static ssize_t mpu6500_read(FAR struct file *filep, FAR char *buffer,
                              size_t buflen);
static ssize_t mpu6500_write(FAR struct file *filep,
                               FAR const char *buffer, size_t buflen);
static int mpu6500_ioctl(FAR struct file *filep, int cmd,
                           unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_mpu6500_fops =
{
  NULL,             /* open */
  NULL,             /* close */
  mpu6500_read,   /* read */
  mpu6500_write,  /* write */
  NULL,             /* seek */
  mpu6500_ioctl,  /* ioctl */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mpu6500_configspi
 *
 * Description:
 *   Configure the SPI instance for to match the MPU6500
 *   specifications
 *
 ****************************************************************************/

static inline void mpu6500_configspi(FAR struct spi_dev_s *spi)
{
  SPI_SETMODE(spi, MPU6500_SPI_MODE);
  SPI_SETBITS(spi, 8);
  SPI_HWFEATURES(spi, 0);
  SPI_SETFREQUENCY(spi, CONFIG_MPU6500_SPI_FREQUENCY);
}

/****************************************************************************
 * Name: mpu6500_set_attenuation
 *
 * Description:
 *   Set the attenuation level in dB (16.16 bits fixed point).
 *
 ****************************************************************************/

static void mpu6500_set_attenuation(FAR struct mpu6500_dev_s *priv,
                                      b16_t attenuation)
{
  SPI_LOCK(priv->spi, true);

  mpu6500_configspi(priv->spi);

  SPI_SELECT(priv->spi, priv->spidev, false);

  /* Convert the attenuation value from 16.16 bits to 5.1 bits. */

  SPI_SEND(priv->spi, (uint8_t)(attenuation >> 15));

  /* Send a pulse to the LE pin */

  SPI_SELECT(priv->spi, priv->spidev, true);
  up_udelay(1);
  SPI_SELECT(priv->spi, priv->spidev, false);

  SPI_LOCK(priv->spi, false);
}

/****************************************************************************
 * Name: mpu6500_write
 *
 * Description:
 *   Write is not permitted, only IOCTLs.
 ****************************************************************************/

static ssize_t mpu6500_write(FAR struct file *filep,
                               FAR const char *buffer,
                               size_t buflen)
{
  return -ENOSYS;
}

/****************************************************************************
 * Name: mpu6500_read
 *
 * Description:
 *   Read is ignored.
 ****************************************************************************/

static ssize_t mpu6500_read(FAR struct file *filep, FAR char *buffer,
                              size_t buflen)
{
  return 0;
}

/****************************************************************************
 * Name: mpu6500_ioctl
 *
 * Description:
 *   The only available ICTL is RFIOC_SETATT. It expects a struct
 *   attenuator_control* as the argument to set the attenuation
 *   level. The channel is ignored as the MPU6500 has just a
 *   single attenuator.
 ****************************************************************************/

static int mpu6500_ioctl(FAR struct file *filep,
                           int cmd,
                           unsigned long arg)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct mpu6500_dev_s *priv = inode->i_private;
  int ret = OK;

  switch (cmd)
    {
      case RFIOC_SETATT:
        {
          FAR struct attenuator_control *att =
            (FAR struct attenuator_control *)((uintptr_t)arg);
          DEBUGASSERT(att != NULL);
          mpu6500_set_attenuation(priv, att->attenuation);
        }
        break;

      default:
        sninfo("Unrecognized cmd: %d\n", cmd);
        ret = -ENOTTY;
        break;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mpu6500_register
 *
 * Description:
 *   Register the mpu6500 character device as 'devpath'.
 *
 ****************************************************************************/

int mpu6500_register(FAR const char *devpath,
                       FAR struct spi_dev_s *spi,
                       int spidev)
{
  FAR struct mpu6500_dev_s *priv;
  int ret;

  /* Sanity check */

  DEBUGASSERT(spi != NULL);

  /* Initialize the MPU6500 device structure */

  priv = (FAR struct mpu6500_dev_s *)
      kmm_malloc(sizeof(struct mpu6500_dev_s));
  if (priv == NULL)
    {
      snerr("ERROR: Failed to allocate instance\n");
      return -ENOMEM;
    }

  priv->spi    = spi;
  priv->spidev = spidev;

  /* Clear the LE pin */

  SPI_SELECT(priv->spi, priv->spidev, false);

  /* Register the character driver */

  ret = register_driver(devpath, &g_mpu6500_fops, 0666, priv);
  if (ret < 0)
    {
      snerr("ERROR: Failed to register driver: %d\n", ret);
      kmm_free(priv);
    }

  return ret;
}

#endif
