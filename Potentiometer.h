#pragma once

#include "daisy_seed.h"

class Potentiometer
{
public:
    Potentiometer()
    :_thresh{0.01f},
    _reverse{false}{}
    ~Potentiometer() {}

    void init(daisy::DaisySeed* hw, int adc_channel_id)
    {
        _hw = hw;
        _adc_channel_id = adc_channel_id;

        _val = _hw->adc.GetFloat(_adc_channel_id);

    }

    void init(daisy::DaisySeed* hw, int adc_channel_id, bool reverse)
    {
        init(hw, adc_channel_id);
        _reverse = reverse;
    }

    // returns true if value changed
    bool process()
    {
        bool output{false};
        float new_val{};
        // if reverse is on read value of 1.0f is evauluated to 0.0f and 0.0f to 1.0f
        if (_reverse) {new_val = 1.0f - _hw->adc.GetFloat(_adc_channel_id);}
        else {new_val = _hw->adc.GetFloat(_adc_channel_id);}
        if (std::abs(new_val - _val) > _thresh) 
        {
            _val = new_val;
            if (new_val > 1.0 - _thresh * 2.0f) {_val = 1.0f;}
            else if (new_val < _thresh *2.0f) {_val = 0.0f;}
            
            output = true;
        }
        return output;
    }

    float getVal() const {return _val;}

private:
    daisy::DaisySeed* _hw{};
    int _adc_channel_id{};
    float _val{};

    float _thresh{}; //> the difference between current potentiameter value and previous that would result in update, set to 0.0 for no threhold
    bool _reverse{};
};
