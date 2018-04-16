#pragma once

#include <AP_HAL/AP_HAL.h>
#include "AP_HAL_Linux.h"

#include <AP_HAL/I2CDevice.h>

#define I2C_BLCTRL_BASEADDR 0x29


namespace Linux {

class RCOutput_BLCTRL : public AP_HAL::RCOutput {
public:
    RCOutput_BLCTRL(AP_HAL::OwnPtr<AP_HAL::I2CDevice> dev);
    ~RCOutput_BLCTRL();
    void init();
    void set_freq(uint32_t chmask, uint16_t freq_hz)override;
    uint16_t get_freq(uint8_t ch)override;
    void enable_ch(uint8_t ch)override;
    void disable_ch(uint8_t ch)override;
    void write(uint8_t ch, uint16_t period_us)override;
    uint16_t read(uint8_t ch)override;
    void read(uint16_t *period_us, uint8_t len)override;
    void cork(void) override;
    void push(void) override;

private:
    static const uint8_t channel_count = 4;
    AP_HAL::OwnPtr<AP_HAL::I2CDevice> _dev;
    uint16_t enable_mask;
    uint16_t period[channel_count];
    virtual void timer_tick(void)override;
    bool corked;
};

}

