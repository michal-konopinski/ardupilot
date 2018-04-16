/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  this is a driver for RC output in the QFLIGHT board. Output goes via
  a UART with a CRC. See libraries/RC_Channel/examples/RC_UART for an
  example of the other end of this protocol
 */
#include <AP_HAL/AP_HAL.h>


#include "RCOutput_BLCtrl.h"

#include <utility>
#include <stdio.h>



extern const AP_HAL::HAL& hal;

using namespace Linux;


RCOutput_BLCTRL::RCOutput_BLCTRL(AP_HAL::OwnPtr<AP_HAL::I2CDevice> dev):_dev(std::move(dev))
{
//    printf("RCOutput_BLCTRL::RCOutput_BLCTRL()");
}
RCOutput_BLCTRL::~RCOutput_BLCTRL()
{
//    printf("RCOutput_BLCTRL::~RCOutput_BLCTRL()");
}




void RCOutput_BLCTRL::init()
{
//    printf("RCOutput_BLCTRL::init()");
    hal.scheduler->register_timer_process(FUNCTOR_BIND_MEMBER(&RCOutput_BLCTRL::timer_tick, void));
    //hal.scheduler->register_io_process(FUNCTOR_BIND_MEMBER(&RCOutput_BLCTRL::timer_tick, void));
}


void RCOutput_BLCTRL::set_freq(uint32_t chmask, uint16_t freq_hz)
{
    // no support for changing frequency
}

uint16_t RCOutput_BLCTRL::get_freq(uint8_t ch)
{
    // return fixed fake value
    return 490;
}

void RCOutput_BLCTRL::enable_ch(uint8_t ch)
{
    if (ch >= channel_count) {
        return;
    }
//    printf("enable ch=%d\n",ch);
    enable_mask |= 1U<<ch;
}

void RCOutput_BLCTRL::disable_ch(uint8_t ch)
{
    if (ch >= channel_count) {
        return;
    }
//    printf("disable ch=%d\n",ch);
    enable_mask &= ~1U<<ch;
}

void RCOutput_BLCTRL::write(uint8_t ch, uint16_t period_us)
{
    if (ch >= channel_count) {
        return;
    }
//    if(period_us!=period[ch])printf("ch=%d val=%d\n",ch,period_us);
    period[ch] = period_us;
}

uint16_t RCOutput_BLCTRL::read(uint8_t ch)
{
    if (ch >= channel_count) {
        return 0;
    }
    return period[ch];
}

void RCOutput_BLCTRL::read(uint16_t *period_us, uint8_t len)
{
    for (int i = 0; i < len; i++) {
        period_us[i] = read(i);
    }
}

#define MIN_PULSEWIDTH 1100
#define MAX_PULSEWIDTH 1900
#define BLCTRL_SCALE_1BYTE 247
#define BLCTRL_SCALE_2BYTE 2047
#define BLCTRL_SCALE BLCTRL_SCALE_2BYTE

void RCOutput_BLCTRL::timer_tick(void)
{
    if (!_dev->get_semaphore()->take_nonblocking()) {
//    if (!_dev->get_semaphore()->take(10)) {
        return;
    }

    struct PACKED ch_value {
        uint8_t data_hi;
        uint8_t data_lo;
    } ch_value;

    for(int m=0;m<channel_count;++m)
    {
	if(enable_mask&(1<<m))
	{
	    _dev->set_address(I2C_BLCTRL_BASEADDR + m);
            uint32_t p=period[m];
            if(p<MIN_PULSEWIDTH)p=MIN_PULSEWIDTH;
            p-=MIN_PULSEWIDTH;
            p*=BLCTRL_SCALE;
            p/=MAX_PULSEWIDTH-MIN_PULSEWIDTH;
#if BLCTRL_SCALE == BLCTRL_SCALE_2BYTE
            ch_value.data_hi=p/8;
            ch_value.data_lo=p%8;
            _dev->transfer((uint8_t *)&ch_value, 2, nullptr, 0);
#else
            ch_value.data_hi=p;
            _dev->transfer((uint8_t *)&ch_value, 1, nullptr, 0);
#endif
	}
    }
    _dev->get_semaphore()->give();
}


void RCOutput_BLCTRL::cork(void)
{
    corked = true;
}

void RCOutput_BLCTRL::push(void)
{
    corked = false;
}


