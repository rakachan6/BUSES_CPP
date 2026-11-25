#ifndef BUSES_CPP_TIM_H
#define BUSES_CPP_TIM_H
#include "stm32f4xx.h"


class tim
{
public:
    tim(TIM_HandleTypeDef& TIMx) : TIMx_(TIMx) {};
    void blocking_delay(uint32_t time)
    {
        __HAL_TIM_SET_COUNTER(&TIMx_, 0);

        while (__HAL_TIM_GET_COUNTER(&TIMx_) < time)
        {}
    }
    void start_it()
    {
        HAL_TIM_Base_Start_IT(&TIMx_);
    }

private:
    TIM_HandleTypeDef& TIMx_;
};


#endif //BUSES_CPP_TIM_H
