#pragma once

#include "daisy_seed.h"

class Potentiometer
{
public:
    Potentiometer()
    :_thresh{0.01f}{}
    ~Potentiometer() {}

    void init(daisy::DaisySeed* hw, int adc_channel_id)
    {
        _hw = hw;
        _adc_channel_id = adc_channel_id;

        _val = _hw->adc.GetFloat(_adc_channel_id);
    }
    // returns true if value changed
    bool process()
    {
        float new_val = _hw->adc.GetFloat(_adc_channel_id);
        if (std::abs(new_val - _val) > _thresh) 
        {
            _val = new_val;
            if (new_val > 1.0 - _thresh) {_val = 1.0f;}
            else if (new_val < _thresh) {_val = 0.0f;}
            return true;
        }
        return false;
    }

    float getVal() const {return _val;}

private:
    daisy::DaisySeed* _hw{};
    int _adc_channel_id{};
    float _val{};

    float _thresh{}; //> the difference between current potentiameter value and previous that would result in update, set to 0.0 for no threhold
};