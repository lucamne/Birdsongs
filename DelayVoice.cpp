#include "DelayVoice.h"

void DelayVoice::init(float* buffer, int buffer_size, int sample_rate)
{
    _dline = buffer;
    _max_delay = buffer_size;
    _sample_rate = sample_rate;
    // zero out delay line
    for (int i{0}; i < buffer_size; i++)
    {
        _dline[i] = 0.0f;
    }
    // set write pointer to beginning of delay line
    _wptr = _dline;
    // init dsp objects
    _noise.Init();
    _filter.Init(_sample_rate);
    _filter.SetFreq(200.0f);    //> set cutoff point for low pass filter at 200Hz
    // init sin osc for ping pong mode
    const float rate {0.6f + getLPNoise() * 0.5f};
    _sin_osc.Init(_sample_rate);
    _sin_osc.SetFreq(rate);
}

void DelayVoice::process(float in)
{
    // process flutter
    processFlutter();
    
    // clear buffers
    _lbuff = 0.0f;
    _rbuff = 0.0f;

    // calculate current delay based on read and write pointer positions
    float current_delay {static_cast<float>(_wptr - _dline) - _rptr};
    if (current_delay <= 0.0f) {current_delay += static_cast<float>(_max_delay);}   //> enforce positive delay
    // get difference from expected delay and use that value to adjust interpolation amount
    const float delay_diff {current_delay - _delay_time};
    float current_interp{(delay_diff * 1.25f) / static_cast<float>(_sample_rate)};
    if (isinf(current_interp)) {current_interp = 0.0f;}
    
    // read sample from delay line
    float dline_sample {readSample(_rptr + current_interp)};
    // increment read pointer
    _rptr += 1 + current_interp;
    // ensure read pointer in range
    if (static_cast<int>(std::floor(_rptr)) >= _max_delay) {_rptr -= static_cast<float>(_max_delay);}

    // get pan dependent on ping pong mode
    float current_pan{_pan};
    if (_ping_pong_mode) {current_pan = _sin_osc.Process() + 0.5f;}
    // if not in ping pong mode, still run osc
    else {_sin_osc.Process();}

    // set voice output 
    _lbuff = (1.0f - current_pan) * dline_sample;
    _rbuff = current_pan * dline_sample;

    // write new sample to delay line
    *_wptr = in + (_lbuff + _rbuff) * _feedback;

    // increment write pointer and keep in range
    if (++_wptr - _dline >= _max_delay) {_wptr -= _max_delay;}
}

void DelayVoice::setDelayTime(float samples)
{
    // ensure samples is in range
    if (samples >= static_cast<float>(_max_delay)) {samples = static_cast<float>(_max_delay) - 1.0f;}
    else if (samples < 0.01f) {samples = 0.01f;}

    _delay_time = samples;
}

void DelayVoice::setFeedback(float feedback)
{
    if (feedback < 0.0f) {feedback = 0.0f;}
    else if (feedback > 1.0f) {feedback = 1.0f;}

    _feedback = feedback;
}

void DelayVoice::setPan(float pan)
{
    if (pan < 0.0f) {pan = 0.0f;}
    else if (pan > 1.0f) {pan = 1.0f;}

    _pan = pan;

    // adjust phase of ping pong osc based on _pan
    _sin_osc.PhaseAdd(_pan * 0.5f);
}

void DelayVoice::setFlutter(float flutter)
{
    if (flutter < 0.0f) {flutter = 0.0f;}
    else if (flutter > 1.0f) {flutter = 1.0f;}

    _flutter = flutter;
}

float DelayVoice::readSample(float position)
{
    // get samples to be interpolated
    float interp_amnt{position - std::floor(position)};
    int samp1{static_cast<int>(std::floor(position))};
    int samp2{samp1 + 1};
    // ensure samples are within bounds
    if (samp1 < 0) {samp1 += _max_delay;}
    else if (samp1 >= _max_delay) {samp1 -= _max_delay;}
    if (samp2 < 0) {samp2 += _max_delay;}
    else if (samp2 >= _max_delay) {samp2 -= _max_delay;}
    // if interp amount is very large or very small than round
    if (interp_amnt < (1.0f / static_cast<float>(_max_delay))) { interp_amnt = 0.0f;}
    else if (interp_amnt > (static_cast<float>(_max_delay - 1) / static_cast<float>(_max_delay))) {interp_amnt = 1.0f;}

    return (1.0f - interp_amnt) * _dline[samp1] + interp_amnt * _dline[samp2];
}

void DelayVoice::processFlutter()
{
    static constexpr float DELAY_SCALAR{10.0f};
    static constexpr float LEVEL_SCALAR{0.07f}; 
    // randomizing delay time slightly causes pleasent random pitch shifting
    float noise {getLPNoise()};
    setDelayTime(_delay_time + _flutter * DELAY_SCALAR * noise);
    // randomize delay volume
    noise = std::abs(getLPNoise());
    _level = 1.0f - (noise * _flutter * LEVEL_SCALAR);
}

float DelayVoice::getLPNoise()
{
    const float noise_out{_noise.Process()};
    _filter.Process(noise_out);
    return _filter.Low();
}