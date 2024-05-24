#pragma once

#include "daisysp.h"

class DelayVoice
{
public:
    DelayVoice()
    :_dline{nullptr},
    _max_delay{0},
    _sample_rate{48000},
    _wptr{nullptr},
    _rptr{0.0f},
    _lbuff{0.0f},
    _rbuff{0.0f},
    _delay_time{0.0f},
    _level{1.0f},
    _feedback{0.0f},
    _pan{0.5f},
    _flutter{0.0f},
    _bypass{false} {}

    ~DelayVoice() {}

    void init(float* buffer, int buffer_size, int sample_rate);
    // input new sample
    void process(float in);
    
    // set member values
    // set delay time in samples (samples can be fractional)
    void setDelayTime(float samples);
    // set feedback in range 0.0f to 1.0f
    void setFeedback(float feedback);
    // set pan: 0.0f = left, 1.0f = right
    void setPan(float pan);
    // set flutter amount from range 0.0f to 1.0f
    void setFlutter(float flutter);
    // set bypass to true or false
    void setBypass(bool b) {_bypass = b;}
    
    // get buffer outputs
    float getRight() const {return _rbuff;}
    float getLeft() const {return _lbuff;}
    // get member values
    // returns delay time in samples
    float getDelayTime() const {return _delay_time;}
    // returns feedback
    float getFeedback() const {return _feedback;}
    // returns pan
    float getPan() const {return _pan;}
    // returns flutter
    float getFlutter() const {return _flutter;}
    // returns bypass state
    bool getBypass() const {return _bypass;}

private:
    // delay line members
    float* _dline{};        //> delay line
    int _max_delay{};       //> max delay size in samples
    int _sample_rate{};     //> holds hardware sample rate
    float* _wptr{};         //> delay line write pointer
    float _rptr{};          //> fractional delay line read pointer
    // audio output members
    float _lbuff{};         //> left audio buffer
    float _rbuff{};         //> right audio buffer
    // parameter members
    float _delay_time{};    //> holds target delay time in samples
    float _level{};         //> output level of delay voice
    float _feedback{};      //> delay feedback
    float _pan{};           //> 0.0f is left, 1.0f is right
    float _flutter{};       //> controls warping of delay line
    bool _bypass{};         //> stores bypass state to be used by wrapper
    // daisy premade dsp objects
    daisysp::WhiteNoise _noise{};
    daisysp::Svf _filter{};

    // returns interpolated sample at position in _dline
    float readSample(float position);
    // randomly alters delay time to cause warping and adds some low freq noise
    void processFlutter();
    // returns low freq noise
    float getLPNoise();
};