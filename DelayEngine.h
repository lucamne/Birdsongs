#pragma once

#include "DelayVoice.h"

class DelayEngine
{
public:
    DelayEngine()
    :_voices{nullptr} {}

    ~DelayEngine() { delete[] _voices; delete[] _ratios;}

    // initializes engines with max delay per voice, number of voices and sample rate -- Ensure buffer size is >= max_delay * voice_count
    void init(float* buffer1, float* buffer2, int max_delay, int num_voices, int sample_rate);
    // processes new sample
    void process(float left, float right);
    void process(float in) {process(in * 0.5f, in * 0.5f);}
    // get stereo output
    float getLeft();
    float getRight();

    /// setters

    // sets master delay time in samples
    void setMasterDelayTime(float samples);
    // set master feedback in range 0.0f to 1.0f
    void setMasterFeedback(float feedback);
    // sets master flutter in range 0.0f to 1.0f
    void setMasterFlutter(float flutter);
    // set ping pong mode to on or off
    void setPingPongMode(bool b);
    // set delay ratio of specific voice in range 0.0f to 1.0f
    void setDelayRatio(int voice_id, float ratio);
    // set pan of specific voice in range 0.0f to 1.0f
    void setPan(int voice_id, float pan);
    // set bypass of specific voice to true or false
    void setBypass(int voice_id, bool b) {_voices[voice_id].setBypass(b);}
    // set detune in samples to stretch
    void setDetune(int voice_id, float detune) {_voices[voice_id].setDetune(detune);}

    /// getters

    // returns delay time in samples
    float getMasterDelayTime() const {return _master_delay_time;}
    // returns feedback in range 0.0f to 1.0f
    float getMasterFeedback() const {return _master_feedback;}
    // returns flutter in range 0.0f to 1.0f
    float getMasterFlutter() const {return _master_flutter;}
    // returns voice count
    int getVoiceCount() const {return _voice_count;}

private:
    DelayVoice* _voices{};
    int _voice_count{};
    int _max_delay{};           //> max delay time in samples determines how much space is to be allocated per voice
    float* _ratios{};           //> ratios of per voice delay time to _master_delay_time
    // control parameters
    float _master_delay_time{}; //> master delay time in samples
    float _master_feedback{};
    float _master_flutter{};

    // ensures that x is between 0.0f and 1.0f
    float enforceRatio(float x);
};