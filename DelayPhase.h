#pragma once

#include "daisysp.h"

struct DelayVoice
{
    float _rptr{0.0f};
    float _ratio{1.0f}; //> ratio of master delay time
    float _delay_time{}; //> target delay time in samples
    float _inter_amnt{}; // holds current interplation amount
    float _pan{0.5f}; //> 0.0 is left and 1.0 is right
    float _level{1.0f};
    float* _dline{};
};


template <int MAX_DELAY, int SAMPLE_RATE>
class DelayPhase
{
public:
    DelayPhase();

    ~DelayPhase() {delete[] _voices;}
    // adds new sample to delay line
    void process(float in);
    // initializes delayline
    void init();
    // returns sample in left buffer
    float getLeft() const {return _lbuff;}
    //returns sample in right buffer
    float getRight() const {return _rbuff;}
    // add new voice to delay
    void addVoice();
    // master delay phase controls - range: 0.0f - 1.0f
    void setMasterDelay(float time);  
    void setFeedback(float f) {_feedback = f;}
    void setWarble(float w) {_warble = w;}
    // voice specific controls - range: 0.0f - 1.0f
    void setRatio(int voice_id, float ratio) {_voices[voice_id]._ratio = ratio;}
    void setPan(int voice_id, float pan) {_voices[voice_id]._pan = pan;}
    void setLevel(int voice_id, float lvl) {_voices[voice_id]._level - lvl;}
private:
    int _voice_count{};
    //float _dline[MAX_DELAY]{}; //> main delay line in ram
    float DSY_SDRAM_BSS _dline_mem[MAX_DELAY * 50]; //> to replace _dline. delay line buffer stored in sdram

    float* _wptr{}; //> write pointer
    DelayVoice* _voices{};

    float _master_delay{}; //> master delay time that voice delay ratio is based on 
    float _feedback{}; //> overall feed back of system
    float _warble{}; //> random delay shift which also caused pith shifting, higher is more unstable

    float _lbuff{};
    float _rbuff{};

    daisysp::WhiteNoise _noise{};
    daisysp::Svf _filter{};

    // set delay time per voice
    void setDelayTime(int voice_id, float samples);

    // reads sample from delay line at specific position and interpolates
    float readSample(float position);

    void randomizeDelays();
};

template <int MAX_DELAY, int SAMPLE_RATE>
DelayPhase<MAX_DELAY, SAMPLE_RATE>::DelayPhase()
:_voice_count{0},
//_wptr{_dline},
_voices{new DelayVoice[_voice_count]},
_master_delay{0.5f},
_feedback{0.3f},
_warble{0.5f} 
{
    setMasterDelay(_master_delay);
    // init dsp effects
    _noise.Init();
    _filter.Init(SAMPLE_RATE);
    _filter.SetFreq(1000.0f);
}

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
        float curr_delay {static_cast<float>(_wptr - _dline) - _voices[i]._rptr};
        // enforce positive delay
        if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
        // get difference from expected delay
        const float delay_diff {curr_delay - _voices[i]._delay_time};
        float interp_amnt{};

        if (std::abs(delay_diff) < std::abs(_voices[i]._inter_amnt)) {interp_amnt = 0.0f;}
        else {interp_amnt = _voices[i]._inter_amnt;}

        // read value and interpolate if necassary to lengthen or shorten delay
        const float read_sample{readSample(_voices[i]._rptr + interp_amnt) * _voices[i]._level};
        
        _voices[i]._rptr += 1 + interp_amnt;
        // keep read pointer in range
        if (static_cast<int>(std::floor(_voices[i]._rptr)) >= MAX_DELAY) {_voices[i]._rptr -= static_cast<float>(MAX_DELAY);}

        _lbuff += (1.0f - _voices[i]._pan) * read_sample;
        _rbuff += _voices[i]._pan * read_sample;
    }

    // write new sample to delay line
    *(_wptr++) = in + (_lbuff + _rbuff) * 0.5f * _feedback;
    // keep write pointer in range
    if (_wptr - _dline >= MAX_DELAY) {_wptr -= MAX_DELAY;}
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::init()
{
    for (int i{0}; i < MAX_DELAY * 50; i ++)
    {
        _dline_mem[i] = 0.0f;
    }

    _wptr = _dline_mem;
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::addVoice()
{
    DelayVoice* new_arr {new DelayVoice[++_voice_count]};
    memcpy(new_arr,_voices,(_voice_count - 1) * sizeof(DelayVoice));

    delete[] _voices;
    _voices = new_arr;

    _voices[_voice_count - 1]._dline = _dline + (_voice_count - 1) * MAX_DELAY;
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY, SAMPLE_RATE>::setMasterDelay(float time)
{
    _master_delay = time * static_cast<float>(MAX_DELAY);
    if (_master_delay < static_cast<float>(_voice_count)) {_master_delay = static_cast<float>(_voice_count);}

    for (int i{0}; i < _voice_count; i++)
    {
        setDelayTime(i,_voices[i]._ratio * _master_delay);
    }
} 

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::setDelayTime(int voice_id, float samples)
{
    if (samples >= static_cast<float>(MAX_DELAY)) {samples = static_cast<float>(MAX_DELAY) - 1.0f;}
    else if (samples < 0.01f) {samples = 0.01f;}

    _voices[voice_id]._delay_time = samples;
    // get current delay time
    float curr_delay {static_cast<float>(_wptr - _dline) - _voices[voice_id]._rptr};
    // enforce positive delay
    if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
    // get difference from expected delay
    const float delay_diff {curr_delay - _voices[voice_id]._delay_time};
    _voices[voice_id]._inter_amnt = delay_diff / static_cast<float>(SAMPLE_RATE);
}

template <int MAX_DELAY, int SAMPLE_RATE>
float DelayPhase<MAX_DELAY,SAMPLE_RATE>::readSample(float position)
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

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::randomizeDelays()
{
    for (int i{0}; i < _voice_count; i++)
    {
        const float noise_out{_noise.Process()};
        _filter.Process(noise_out);
        const float filter_out {_filter.Low()};
        setDelayTime(i,_voices[i]._delay_time + _warble * 10.0f * filter_out);
    }
}