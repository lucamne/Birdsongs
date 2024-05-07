#pragma once

#include "daisy_seed.h"

class Encoder
{
public:
    Encoder(){}
    ~Encoder(){}

    void init(daisy::GPIO* a, daisy::GPIO* b)
    {
        _a = a;
        _b = b;

        _previous_a_state = _a->Read();
        _curr_a_state = _previous_a_state;
    }
    // returns 1, -1, or 0 based on encoder movement
    int process()
    {
        _curr_a_state = _a->Read();
        if (_curr_a_state != _curr_a_state && _curr_a_state)
        {
            // clockwise rotation
            if (_curr_a_state != _b->Read()) {return 1;}
            // counterclockwise rotation
            else {return -1;}
        }
        return 0;
    }
    
private:
    daisy::GPIO* _a{};
    daisy::GPIO* _b{};

    bool _previous_a_state{};
	bool _curr_a_state{};
};