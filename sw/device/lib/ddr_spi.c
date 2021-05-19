#include "sw/device/lib/ddr_spi.h"

#include "sw/device/lib/testing/check.h"
#include "sw/device/lib/runtime/log.h" //for debugging only


const uint32_t kDlaDdrBurstMax = DLA_DDR_BURST_MAX; //bytes
const uint32_t kSpiRxfSize = SPI_RXF_SIZE; //bytes

//src: source array
//len: number of bytes
void convert_endian(void * src, size_t len) {
  //reverse byte-order
  //for each src[i]
  //before: 0x 67 45 23 01 
  //after:  0x 01 23 45 67
  for (uint32_t i=0; i<(len>>2); i++) {
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+1]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+2]);
    //LOG_INFO("DMEM i %d: %x.", i, ((uint8_t *)src)[i*4+3]);
    ((uint32_t *)src)[i] =  ((uint8_t *)src)[i*4]   << 24 |
                            ((uint8_t *)src)[i*4+1] << 16 |
                            ((uint8_t *)src)[i*4+2] << 8  |
                            ((uint8_t *)src)[i*4+3];
  }
}


void write_to_ddr_from_spi(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t write_bytes){
  uint32_t num_spi_patch = write_bytes / kSpiRxfSize;
  uint32_t buffer[kSpiRxfSize/4]; //used for receiving from spi
  for(uint32_t i =0; i<num_spi_patch; i++){
    
    size_t spi_len=0, spi_total_recv=0;
    uint32_t ddr_start_addr = i*(kSpiRxfSize/8); //64-bit/double-word addressable
    
    //Prepare echo
    size_t bytes=0; 
    //SPI will reverse back the byte-order
    uint32_t echo_word = buffer[0] ^ 0x01010101; 
    CHECK(dif_spi_device_send(spi, &echo_word, sizeof(uint32_t), &bytes) == kDifSpiDeviceOk);
    // LOG_INFO("Echo send: %d bytes, 0x%x to SPI host.", bytes, echo_word);
    
    LOG_INFO("Write Patch %d: waiting for SPI input.", i);
    while (spi_total_recv < kSpiRxfSize){
      CHECK(dif_spi_device_recv(spi, (uint8_t *)buffer+spi_total_recv, sizeof(buffer)-spi_total_recv, &spi_len) ==
            kDifSpiDeviceOk);
      
      if (spi_len > 0){
        spi_total_recv += spi_len;
        //LOG_INFO("SPI has received %d bytes.", spi_len);
      }
    }
    
    // LOG_INFO("SPI has received total %d bytes.", spi_total_recv);
    if (spi_total_recv != kSpiRxfSize) LOG_INFO("ERROR! TOTAL RECEIVED BYTES IS NOT %d.", kSpiRxfSize);
    
    convert_endian(buffer, spi_total_recv);
    //for (uint32_t i=0; i<(spi_total_recv>>2); i++) {
    //  LOG_INFO("SPI buffer i %d: 0x%x.", i, buffer[i]);
    //}
    
    CHECK(dif_dla_ddr_ctrl_write_buf(dla, ddr_start_addr, buffer, spi_total_recv)== kDifDlaOk);
    // LOG_INFO("DDR has written the buffer content.");
    
  }
}

void read_from_ddr_to_spi(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t ddr_addr, uint32_t read_bytes){
  uint32_t bytes_left = read_bytes;
  uint32_t patch=0;
  
  while (bytes_left>0) {
    uint32_t dest_ddr[kDlaDdrBurstMax/4];
    uint32_t wptr=0;
    uint32_t bytes_read = (bytes_left>kDlaDdrBurstMax) ?kDlaDdrBurstMax :bytes_left;
    uint32_t ddr_start_addr = patch*(kDlaDdrBurstMax/8) + ddr_addr; //64-bit/double-word addressable
    // LOG_INFO("Patch %d, total left: %d, read %d, ddr_start_addr %d.", patch, bytes_left, bytes_read, ddr_start_addr);
    
    CHECK(dif_dla_ddr_ctrl_read(dla, ddr_start_addr, dest_ddr, bytes_read, &wptr)== kDifDlaOk);
    // LOG_INFO("DMEM write pointer is at %d.", wptr);


    //send to SPI
    LOG_INFO("Read Patch %d: Writing to SPI.", patch);
    size_t spi_len=0, spi_total_send=0;
    while (spi_total_send < bytes_read){
      CHECK(dif_spi_device_send(spi, (uint8_t *)dest_ddr+spi_total_send, sizeof(dest_ddr)-spi_total_send, &spi_len) ==
            kDifSpiDeviceOk);
      
      if (spi_len > 0){
        spi_total_send += spi_len;
        //LOG_INFO("SPI has sent %d bytes, remaining %d bytes", spi_len, sizeof(dest_ddr)-spi_total_send);
      }
    }
    /*
    for (uint32_t i=0; i<(bytes_read>>3); i++) {
      LOG_INFO("DMEM i %d: 0x %x %x.", i+patch*(kDlaDdrBurstMax/8), dest_ddr[i*2+1], dest_ddr[i*2]);
    }
    LOG_INFO(" ");//*/
    
    bytes_left -= bytes_read;
    patch++;
  }
}

void ddr_test(const dif_dla_t *dla, const dif_spi_device_t *spi, uint32_t ddr_bytes) {
  write_to_ddr_from_spi(dla, spi, ddr_bytes);
  read_from_ddr_to_spi(dla, spi, 0, ddr_bytes);
}