#include <stdio.h>
#include "driver/i2c.h"
#include "esp_system.h"


#define I2C_MASTER_SCL_IO           7      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define STM_SENSOR_ADDR                 0x30        /*!< Slave address of the MPU9250 sensor */
#define MPU9250_RESET_BIT                   7

 esp_err_t i2c_master_init(void);
 esp_err_t stm32_register_write_byte(uint8_t reg_addr, uint8_t data);
 esp_err_t stm32_register_read(uint8_t reg_addr, uint8_t *data, size_t len);