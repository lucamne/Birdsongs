#pragma once

#include "daisy_seed.h"

class Potentiameter
{
public:
    Potentiameter()
    :_change_threshold{0.01f},
    _upper_threhold{0.99f},
    _lower_threhold{0.01f} {}
    ~Potentiameter() {}

    void init(daisy::DaisySeed* hw, int adc_channel_id)
    {
        _hw = hw;
        _adc_channel_id = adc_channel_id;

        _val = _hw->adc.GetFloat(_adc_channel_id);
    }
    // updates val if necassary and returns 
    float process()
    {
        float new_val = _hw->adc.GetFloat(_adc_channel_id);
        if (std::abs(new_val - _val) > _change_threshold) {_val = new_val;}
        else if (new_val > _upper_threhold) {new_val = 1.0f;}
        else if (new_val < _lower_threhold) {new_val = 0.0f;}

        return _val;
    }
private:
    daisy::DaisySeed* _hw{};
    int _adc_channel_id{};
    float _val{};

    float _change_threshold{}; //> the difference between current potentiameter value and previous that would result in update, set to 0.0 for no threhold
    float _lower_threhold{}; //> level below which pot value should be considered 0.0f
    float _upper_threhold{}; //> level above which pot value should be considered 1.0f
};