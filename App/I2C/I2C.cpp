#include "I2C.h"
#include "tim.h"

extern I2C_HandleTypeDef hi2c1;
extern tim tim_1;

I2C_bus i2c_1(hi2c1, tim_1,
            GPIOB, GPIOB,
            GPIO_PIN_6, GPIO_PIN_7,
            GPIO_AF4_I2C1);

I2C_bus::I2C_bus(I2C_HandleTypeDef& i2c_bus,
             tim& recovery_tim,
             GPIO_TypeDef* GPIO_scl,
             GPIO_TypeDef* GPIO_sda,
             uint32_t scl_pin,
             uint32_t sda_pin,
             uint8_t GPIO_AF)
: i2c_bus_(i2c_bus),
  recovery_tim_(recovery_tim),
  GPIO_scl_(GPIO_scl),
  GPIO_sda_(GPIO_sda),
  scl_pin_(scl_pin),
  sda_pin_(sda_pin),
  GPIO_AF_(GPIO_AF)
{}

I2C_status_t I2C_bus::recovery()
{
    HAL_I2C_DeInit(&i2c_bus_);
    pin_release();
    I2C_status_t status;
    status = exec_recover_clock();
    if (status != I2C_status_t::OK)
        return status;

    status = exec_stop_condition();
    if (status != I2C_status_t::OK)
        return status;

    //restore pins and bus
    pin_restore();
    if (HAL_I2C_Init(&i2c_bus_) != HAL_OK)
        return I2C_status_t::ERROR;

    return I2C_status_t::OK;

}

I2C_status_t I2C_bus::transmit_it(uint16_t dev_addr, uint8_t* data, uint16_t length)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Master_Transmit_IT(&i2c_bus_, dev_addr << 1U, data, length);
    return convert_status(status);

}

I2C_status_t I2C_bus::receive_it(uint16_t dev_addr, uint8_t* data, uint16_t length)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Master_Receive_IT(&i2c_bus_, dev_addr << 1U, data, length);
    return convert_status(status);
}

I2C_status_t I2C_bus::mem_write_it(uint16_t dev_addr, uint16_t mem_addr, uint8_t* data, uint16_t length)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write_IT(&i2c_bus_, dev_addr << 1U, mem_addr,
                        I2C_MEMADD_SIZE_8BIT, data, length);

    return convert_status(status);
}

I2C_status_t I2C_bus::mem_read_it(uint16_t dev_addr, uint16_t mem_addr, uint8_t* data, uint16_t length)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read_IT(&i2c_bus_, dev_addr << 1U, mem_addr,
                        I2C_MEMADD_SIZE_8BIT, data, length);

    return convert_status(status);
}

bool I2C_bus::is_stuck()
{
    recovery_tim_.blocking_delay(10);
    GPIO_PinState sda_state = HAL_GPIO_ReadPin(GPIO_sda_, sda_pin_);

    return (sda_state == GPIO_PIN_RESET) ? true : false;
}

void I2C_bus::pin_restore()
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_HIGH;
    gpio.Alternate = GPIO_AF_;

    gpio.Pin = scl_pin_;
    HAL_GPIO_Init(GPIO_scl_, &gpio);
    gpio.Pin = sda_pin_;
    HAL_GPIO_Init(GPIO_sda_, &gpio);
}

void I2C_bus::pin_release()
{
    HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_SET);

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_LOW;

    gpio.Pin = scl_pin_;
    HAL_GPIO_Init(GPIO_scl_, &gpio);
    gpio.Pin = sda_pin_;
    HAL_GPIO_Init(GPIO_sda_, &gpio);
}

I2C_status_t I2C_bus::exec_recover_clock()
{
    for (uint8_t i = 0U; i < 9U; i++)
    {
        if (HAL_GPIO_ReadPin(GPIO_sda_, sda_pin_) == GPIO_PIN_SET)
            break;
        HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_RESET);
        recovery_tim_.blocking_delay(5);
        if (HAL_GPIO_ReadPin(GPIO_scl_, scl_pin_) != GPIO_PIN_RESET)
        {
            HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_SET);
            return I2C_status_t::TIMEOUT;
        }
        HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
        recovery_tim_.blocking_delay(5);
    }
    if (HAL_GPIO_ReadPin(GPIO_sda_, sda_pin_) != GPIO_PIN_SET)
        return I2C_status_t::ERROR;

    return I2C_status_t::OK;
}

I2C_status_t I2C_bus::exec_stop_condition()
{
    HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_RESET);
    recovery_tim_.blocking_delay(5);
    if (HAL_GPIO_ReadPin(GPIO_scl_, scl_pin_) != GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_SET);
        return I2C_status_t::TIMEOUT;
    }
    HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_RESET);
    recovery_tim_.blocking_delay(5);
    HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
    recovery_tim_.blocking_delay(5);
    if (HAL_GPIO_ReadPin(GPIO_scl_, scl_pin_) != GPIO_PIN_SET)
    {
        HAL_GPIO_WritePin(GPIO_scl_, scl_pin_, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_SET);
        return I2C_status_t::TIMEOUT;
    }
    HAL_GPIO_WritePin(GPIO_sda_, sda_pin_, GPIO_PIN_SET);
    recovery_tim_.blocking_delay(5);

    if (HAL_GPIO_ReadPin(GPIO_scl_, scl_pin_) != GPIO_PIN_SET ||
        HAL_GPIO_ReadPin(GPIO_sda_, sda_pin_) != GPIO_PIN_SET)
        return I2C_status_t::ERROR;

    return I2C_status_t::OK;
}

I2C_status_t I2C_bus::convert_status(HAL_StatusTypeDef hal_status)
{
    switch (hal_status)
    {
    case HAL_OK:        return I2C_status_t::OK;
    case HAL_TIMEOUT:   return I2C_status_t::TIMEOUT;
    case HAL_BUSY:      return I2C_status_t::BUSY;
    case HAL_ERROR:     return I2C_status_t::ERROR;
    default:            return I2C_status_t::ERROR;
    }
}