#ifndef BUSES_CPP_I2C_H
#define BUSES_CPP_I2C_H

#include "stm32f4xx.h"
#include "tim.h"

enum class I2C_status_t
{
    OK,
    TIMEOUT,
    BUSY,
    ERROR
};


class I2C_bus
{
public:
    I2C_bus(I2C_HandleTypeDef& i2c_bus,
            tim& recovery_tim,
            GPIO_TypeDef* GPIO_scl,
            GPIO_TypeDef* GPIO_sda,
            uint32_t scl_pin,
            uint32_t sda_pin,
            uint8_t GPIO_AF);

    I2C_status_t recovery();
    I2C_status_t transmit_it(uint16_t dev_addr, uint8_t* data, uint16_t length);
    I2C_status_t receive_it(uint16_t dev_addr, uint8_t* data, uint16_t length);
    I2C_status_t mem_write_it(uint16_t dev_addr, uint16_t mem_addr, uint8_t* data, uint16_t length);
    I2C_status_t mem_read_it(uint16_t dev_addr, uint16_t mem_addr, uint8_t* data, uint16_t length);
private:
    bool is_stuck();
    void pin_restore();
    void pin_release();
    I2C_status_t exec_recover_clock();
    I2C_status_t exec_stop_condition();
    static I2C_status_t convert_status(HAL_StatusTypeDef hal_status);

    I2C_HandleTypeDef& i2c_bus_;
    tim& recovery_tim_;
    GPIO_TypeDef* GPIO_scl_;
    GPIO_TypeDef* GPIO_sda_;
    uint32_t scl_pin_;
    uint32_t sda_pin_;
    uint8_t GPIO_AF_;
};

#endif //BUSES_CPP_I2C_H
