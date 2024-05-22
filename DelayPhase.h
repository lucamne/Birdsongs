#pragma once


#include "daisysp.h"
#include "daisy_seed.h"

struct DelayVoice
{
    DelayVoice()
    :_bypass{false},
    _rptr{0.0f},
    _ratio{1.0f},
    _pan{0.5f},
    _level{1.0f},
    _feedback{0.3f} {}

    bool _bypass{}; //> true if bypassed, false if active
    float _rptr{};
    float _ratio{}; //> ratio of master delay time
    float _delay_time{}; //> target delay time in samples
    float _inter_amnt{}; // holds current interplation amount
    float _pan{}; //> 0.0 is left and 1.0 is right
    float _level{};
    float _feedback{}; //> currently not used
    //float* _dline{};
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
    void init(float* buffer, int size);
    // returns sample in left buffer
    float getLeft() const {return _lbuff;}
    //returns sample in right buffer
    float getRight() const {return _rbuff;}
    // add new voice to delay
    void addVoice();
    void deleteVoice(int voice_id);

    // set master delay time as ratio of MAX_DELAY - range: 0.0f - 1.0f
    void setMasterDelay(float time);  
    void setFlutter(float w) {_flutter = w;}
    // voice specific controls - range: 0.0f - 1.0f
    void setGlobalFeedback(float f) { _master_feedback = f;}
    void setFeedback(int voice_id, float f) {_voices[voice_id]._feedback = f;}
    void setRatio(int voice_id, float ratio) 
    {
        _voices[voice_id]._ratio = ratio;
        setDelayTime(voice_id,_voices[voice_id]._ratio * _master_delay);
    }
    void setPan(int voice_id, float pan) {_voices[voice_id]._pan = pan;}
    void setGlobalLevel(float f) {for (int i{0}; i < _voice_count; i++) {_voices[i]._level = f;}}
    void setLevel(int voice_id, float lvl) {_voices[voice_id]._level = lvl;}
    // bool activates voice, false deactivates
    void setBypass(int voice_id, bool bypass) {_voices[voice_id]._bypass = bypass;}
    // system level getters
    int getVoiceCount() const {return _voice_count;}
    float getMasterDelayTimeInMS() const {return (_master_delay / SAMPLE_RATE) * 1000.0f;}
    float getFlutter() const {return _flutter;}
    float getGlobalFeedback() const {return _master_feedback;}
    // voice specific getters
    float getRatio(int voice_id) const { return _voices[voice_id]._ratio;}
    float getFeedback(int voice_id) const { return _voices[voice_id]._feedback;}
    float getPan(int voice_id) const { return _voices[voice_id]._pan;}
    float getLevel(int voice_id) const {return _voices[voice_id]._level;}

private:
    int _voice_count{};
    float* _dline_mem; //> to replace _dline. delay line buffer stored in sdram
    float* _wptr{}; //> write pointer
    DelayVoice* _voices{};

    float _master_delay{}; //> master delay time in samples that voice delay ratio is based on 
    float _master_feedback{};
    float _flutter{}; //> random delay shift which also caused pith shifting, higher is more unstable

    float _lbuff{};
    float _rbuff{};

    daisysp::WhiteNoise _noise{};
    daisysp::Svf _filter{};

    // set delay time per voice
    void setDelayTime(int voice_id, float samples);

    // reads sample from delay line at specific position and interpolates
    float readSample(float position);
    // returns low pass filtered noise value
    float getLPNoise();
    // randomizes delay values to create a flutter effect
    void setFlutter();
};

template <int MAX_DELAY, int SAMPLE_RATE>
DelayPhase<MAX_DELAY, SAMPLE_RATE>::DelayPhase()
:_voice_count{0},
//_wptr{_dline},
_voices{new DelayVoice[_voice_count]},
_master_delay{1000.0f},
_master_feedback{0.3f},
_flutter{0.5f} 
{
    setMasterDelay(_master_delay);
    // init dsp effects
    _noise.Init();
    _filter.Init(SAMPLE_RATE);
    _filter.SetFreq(200.0f);
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::process(float in)
{
    // apply some randomness to delays
    setFlutter();
    // clear buffers
    _lbuff = 0.0f;
    _rbuff = 0.0f;

    // read next sample from delay line
    for (int i{0}; i < _voice_count; i++)
    {
        // Get current delay time
        float curr_delay {static_cast<float>(_wptr - _dline_mem) - _voices[i]._rptr};
        // enforce positive delay
        if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
        // get difference from expected delay
        const float delay_diff {curr_delay - _voices[i]._delay_time};
        float interp_amnt{};
        // get interpolation amount
        if (std::abs(delay_diff) < std::abs(_voices[i]._inter_amnt)) {interp_amnt = 0.0f;}
        else {interp_amnt = _voices[i]._inter_amnt;}

        // read value and interpolate if necassary to lengthen or shorten delay
        const float read_sample{readSample(_voices[i]._rptr + interp_amnt + MAX_DELAY * i)};
        // increment read pointer
        _voices[i]._rptr += 1 + interp_amnt;
        // keep read pointer in range
        if (static_cast<int>(std::floor(_voices[i]._rptr)) >= MAX_DELAY) {_voices[i]._rptr -= static_cast<float>(MAX_DELAY);}

        // voice output
        const float left_out = (1.0f - _voices[i]._pan) * read_sample * _voices[i]._level;
        const float right_out = _voices[i]._pan * read_sample * _voices[i]._level;

        // write new sample to delay line
        *(_wptr + MAX_DELAY * i) = in + (left_out + right_out) * _master_feedback;

        // if voice is not bypassed then push to output buffer
        if (!_voices[i]._bypass)
        {
            // push sample to buffers with panning
            _lbuff += left_out;
            _rbuff += right_out;
        }
    }

    // move write pointer forward and keep in range
    if (++_wptr - _dline_mem >= MAX_DELAY) {_wptr -= MAX_DELAY;}
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::init(float* buffer, int size)
{
    _dline_mem = buffer;

    for (int i{0}; i < size; i ++)
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
    //for (int i{0}; i < _voice_count - 1; i++) {new_arr[i] = _voices[i];}

    delete[] _voices;
    _voices = new_arr;
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY, SAMPLE_RATE>::deleteVoice(int voice_id)
{
    DelayVoice* new_arr {new DelayVoice[std::max(--_voice_count,0)]};
    memcpy(new_arr, _voices  ,voice_id * sizeof(DelayVoice));
    if (voice_id < _voice_count) {memcpy(new_arr + voice_id, _voices + voice_id + 1, (_voice_count - (voice_id + 1)) * sizeof(DelayVoice));}
    //for (int i{0}; i < voice_id; i++) {new_arr[i] = _voices[i];}
    //for (int i{voice_id + 1}; i < _voice_count; i++) {new_arr[i] = _voices[i];}

    delete[] _voices;
    _voices = new_arr;
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
    float curr_delay {static_cast<float>(_wptr - _dline_mem) - _voices[voice_id]._rptr};
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

    return (1.0f - interp_amnt) * _dline_mem[samp1] + interp_amnt * _dline_mem[samp2];
}

template <int MAX_DELAY, int SAMPLE_RATE>
float DelayPhase<MAX_DELAY, SAMPLE_RATE>::getLPNoise()
{
    const float noise_out{_noise.Process()};
    _filter.Process(noise_out);
    return _filter.Low();
}

template <int MAX_DELAY, int SAMPLE_RATE>
void DelayPhase<MAX_DELAY,SAMPLE_RATE>::setFlutter()
{
    static constexpr float DELAY_SCALAR{10.0f};
    static constexpr float LEVEL_SCALAR{0.05f};

    for (int i{0}; i < _voice_count; i++)
    {   
        // randomizing delay time slightly causes pleasent random pitch shifting
        float noise {getLPNoise()};
        setDelayTime(i,_voices[i]._delay_time + _flutter * DELAY_SCALAR * noise);
        
        // randomize delay volume
        noise = getLPNoise();
        // ensure positive
        noise = std::abs(noise);
        _voices[i]._level = 1.0f - (noise * _flutter * LEVEL_SCALAR);
    }
}
