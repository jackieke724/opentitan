#ifndef OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_SPI_H_
#define OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_SPI_H_

#include <stdint.h>
#include "sw/device/lib/dif/dif_spi_device.h"
#include "sw/device/lib/dif/dif_dla.h"

#define DLA_DDR_BURST_MAX 2048 //bytes
#define SPI_RXF_SIZE 1024 //bytes by default, needs to match the SPI Device registers

/*
assumes ddr write address starts at 0.

write_bytes:  minimum grain-size is 32B/256bits,
              which is the width of DDR on Genesys 2.
              needs to be divisble by 1024,
              since we send patches of 1024B
*/
void write_to_ddr_from_spi(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t write_bytes);

/*
ddr_addr:   64-bit addressable in unsigned decimal
read_bytes: minimum grain-size is 32B/256bits,
            which is the width of DDR on Genesys 2.
            needs to be divisble by 1024,
            since we read patches of 1024B
*/
void read_from_ddr_to_spi(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t ddr_addr, uint32_t read_bytes);

/*
A sanity test for spi <-> ddr
write to ddr from spi, then read from ddr and send to spi
*/
void ddr_test(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t ddr_bytes);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // OPENTITAN_SW_DEVICE_LIB_DIF_DIF_DDR_SPI_H_
