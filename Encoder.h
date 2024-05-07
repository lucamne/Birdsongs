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
    }

    void init(daisy::GPIO* a, daisy::GPIO* b, daisy::GPIO* c)
    {
        init(a,b);
        _c = c;
        _button_state = !c->Read();
    }
    // returns 1, -1, or 0 based on encoder movement
    int process()
    {
        const bool new_read = _a->Read();
        const bool eval{new_read && !_previous_a_state};
        _previous_a_state = new_read;
        if (eval)
        {
            // clockwise rotation
            if (!_b->Read()) {return 1;}
            // counterclockwise rotation
            else {return -1;}
        }
        return 0;
    }

    bool isPressed() 
    {
        _button_state = !_c->Read();
        return _button_state;
    }

    bool isReleased()
    {
        const bool new_read = !_c->Read();
        const bool out {_button_state && !new_read};
        _button_state = new_read;
        return out;
    }

    bool isClicked()  
    {
        const bool new_read = !_c->Read();
        const bool out {!_button_state && new_read};
        _button_state = new_read;
        return out;
    }
    
private:
    daisy::GPIO* _a{};
    daisy::GPIO* _b{};
    daisy::GPIO* _c{};

    bool _previous_a_state{};

    bool _button_state{};
};