#pragma once

#include "daisysp.h"

template <int MAX_DELAY, int SAMPLE_RATE>
class DelayPhase
{
public:
    DelayPhase()
    :_voice_count{3},
    _rptr_arr{new float[_voice_count]},
    _wptr{_dline},
    _delay_times{new float[_voice_count]},
    _inter_arr{new float[_voice_count]},
    _feedback{0.25f},
    _time_spread{0.625f},
    _warble{0.5f} 
    {
        for (int i{0}; i < _voice_count; i++)
        {
            _rptr_arr[i] = 0.0f;
        }

        setDelayTime(MAX_DELAY - 1);

        _noise.Init();
        _filter.Init(SAMPLE_RATE);
        _filter.SetFreq(1000.0f);
    }

    ~DelayPhase()
    {
        delete[] _rptr_arr;
        delete[] _delay_times;
        delete[] _inter_arr;
    }
    // adds new sample to delay line
    void process(float in);
    
    // returns sample in left buffer
    float getLeft() const {return _lbuff;}
    //returns sample in right buffer
    float getRight() const {return _rbuff;}
    // set delay time in samples
    void setDelayTime(int samples) {setDelayTime(static_cast<float>(samples));}
    void setDelayTime(float samples)
    {
        float ratio{1.0f};
        float s{samples};
        if (s < static_cast<float>(_voice_count)) {s = static_cast<float>(_voice_count);}

        for (int i{0}; i < _voice_count; i++)
        {
            setDelayTime(i,ratio * s);
            ratio *= _time_spread;
        }
    }
    void setDelayTime(int voice_id, float samples);
    

    void setFeedback(float f) {_feedback = f;}
    void setWarble(float w) {_warble = w;}

private:
    int _voice_count{};
    float _dline[MAX_DELAY]{};
    float* _rptr_arr{};
    float* _wptr{};
    float* _delay_times{};
    float* _inter_arr{};

    float _feedback{};
    float _time_spread{}; //> controls the variance of delay times
    float _warble{};

    float _lbuff{};
    float _rbuff{};

    daisysp::WhiteNoise _noise{};
    daisysp::Svf _filter{};

    // reads sample from delay line at specific position and interpolates
    float readSample(float position)
    {
        float interp_amnt{position - std::floor(position)};
        int samp1{static_cast<int>(std::floor(position))};
        int samp2{samp1 + 1};

        if (samp1 < 0) {samp1 += MAX_DELAY;}
        else if (samp1 >= MAX_DELAY) {samp1 -= MAX_DELAY;}
        if (samp2 < 0) {samp2 += MAX_DELAY;}
        else if (samp2 >= MAX_DELAY) {samp2 -= MAX_DELAY;}

        if (interp_amnt < (1.0f / static_cast<float>(MAX_DELAY))) { interp_amnt = 0.0f;}
        else if (interp_amnt > (static_cast<float>(MAX_DELAY - 1) / static_cast<float>(MAX_DELAY))) {interp_amnt = 1.0f;}

        return (1.0f - interp_amnt) * _dline[samp1] + interp_amnt * _dline[samp2];
    }

    void randomizeDelays()
    {
        for (int i{0}; i < _voice_count; i++)
        {
            const float noise_out{_noise.Process()};
            _filter.Process(noise_out);
            const float filter_out {_filter.Low()};
            setDelayTime(i,_delay_times[i] + _warble * 10.0f * filter_out);
        }
    }
};

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::process(float in)
{
    // apply some randomness to delays
    randomizeDelays();
    // clear buffers
    _lbuff = 0.0f;
    _rbuff = 0.0f;

    // read next sample from delay line
    for (int i{0}; i < _voice_count; i++)
    {
        // Get current delay time
        float curr_delay {static_cast<float>(_wptr - _dline) - _rptr_arr[i]};
        // enforce positive delay
        if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
        // get difference from expected delay
        const float delay_diff {curr_delay - _delay_times[i]};
        float interp_amnt{};

        if (std::abs(delay_diff) < std::abs(_inter_arr[i])) {interp_amnt = 0.0f;}
        else {interp_amnt = _inter_arr[i];}

        // read value and interpolate if necassary to lengthen or shorten delay
        const float read_sample{readSample(_rptr_arr[i] + interp_amnt)};
        
        _rptr_arr[i]+= 1 + interp_amnt;
        // keep read pointer in range
        if (static_cast<int>(std::floor(_rptr_arr[i])) >= MAX_DELAY) {_rptr_arr[i] -= static_cast<float>(MAX_DELAY);}

        // output to center_dline
        if (i % 3 == 0)
        {
            _lbuff += read_sample / 2.0f;
            _rbuff += read_sample / 2.0f;
        }
        // output left
        else if (i % 3 == 1) {_lbuff += read_sample;}
        // output right
        else {_rbuff += read_sample;}
    }

    // write new sample to delay line
    *(_wptr++) = in + (_lbuff + _rbuff) * 0.5f * _feedback;
    // keep write pointer in range
    if (_wptr - _dline >= MAX_DELAY) {_wptr -= MAX_DELAY;}
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::setDelayTime(int voice_id, float samples)
{
    if (samples >= static_cast<float>(MAX_DELAY)) {samples = static_cast<float>(MAX_DELAY) - 1.0f;}
    else if (samples < 0.01f) {samples = 0.01f;}

    _delay_times[voice_id] = samples;
    // get current delay time
    float curr_delay {static_cast<float>(_wptr - _dline) - _rptr_arr[voice_id]};
    // enforce positive delay
    if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
    // get difference from expected delay
    const float delay_diff {curr_delay - _delay_times[voice_id]};
    _inter_arr[voice_id] = delay_diff / static_cast<float>(SAMPLE_RATE);
}