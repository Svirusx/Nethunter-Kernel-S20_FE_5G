/*====================================================================================*/
/*                                                                                    */
/*                        Copyright 2018 NXP                                          */
/*                                                                                    */
/*   All rights are reserved. Reproduction in whole or in part is prohibited          */
/*   without the written consent of the copyright owner.                              */
/*                                                                                    */
/*   NXP reserves the right to make changes without notice at any time. NXP makes     */
/*   no warranty, expressed, implied or statutory, including but not limited to any   */
/*   implied warranty of merchantability or fitness for any particular purpose,       */
/*   or that the use will not infringe any third party patent, copyright or trademark.*/
/*   NXP must not be liable for any loss or damage arising from its use.              */
/*                                                                                    */
/*====================================================================================*/

#define SR100_MAGIC 0xEA
#define SR100_SET_PWR _IOW(SR100_MAGIC, 0x01, long)
#define SR100_SET_DBG _IOW(SR100_MAGIC, 0x02, long)
#define SR100_SET_POLL _IOW(SR100_MAGIC, 0x03, long)
#define SR100_SET_FWD _IOW(SR100_MAGIC, 0x04, long)
#define SR100_GET_THROUGHPUT _IOW(SR100_MAGIC, 0x05, long)
#define SR100_ESE_RESET _IOW(SR100_MAGIC, 0x06, long)
#define SR100_GET_ANT_CONNECTION_STATUS _IOW(SR100_MAGIC, 0x61, long)

#define NORMAL_MODE_HEADER_LEN 4
#define HBCI_MODE_HEADER_LEN 4
#define NORMAL_MODE_LEN_OFFSET 3
#define UCI_NORMAL_PKT_SIZE 0

#define EXTND_LEN_INDICATOR_OFFSET 1
#define EXTND_LEN_INDICATOR_OFFSET_MASK 0x80
#define EXTENDED_LENGTH_OFFSET 2

struct sr100_spi_platform_data {
  unsigned int irq_gpio;
  unsigned int ce_gpio;
  unsigned int rtc_sync_gpio;
  unsigned int spi_handshake_gpio;
  unsigned int ant_connection_status_gpio;
  unsigned int clk_req_gpio;
  unsigned int clk_req_irq;
  const char *uwb_vdd_io;
  const char *uwb_vdd;
  const char *uwb_vdd_rf;
  const char *uwb_vdd_se_i2c_pullup;
};
enum {
  PWR_DISABLE = 0,
  PWR_ENABLE,
  ABORT_READ_PENDING
};
