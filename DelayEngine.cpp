#include "DelayEngine.h"

void DelayEngine::init(float* buffer1, float* buffer2, int max_delay, int voice_count, int sample_rate)
{
    // zero buffer
    for (int i{0}; i < max_delay * voice_count; i++)
    {
        buffer1[i] = 0;
        buffer2[i] = 0;
    }
    // allocate voice array
    _voices = new DelayVoice[voice_count];
    // init voices
    for (int voice_id{0}; voice_id < voice_count; voice_id++)
    {
        _voices[voice_id].init(buffer1 + max_delay * voice_id, buffer2 + max_delay * voice_id, max_delay, sample_rate);
    }

    // allocate ratio array
    _ratios = new float[voice_count];
    // init all ratios to 1.0f
    for (int voice_id{0}; voice_id < voice_count; voice_id++)
    {
        _ratios[voice_id] = 1.0f;
    }

    // store member variables
    _voice_count = voice_count;
    _max_delay = max_delay;
}

void DelayEngine::process(float left, float right)
{
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        _voices[voice_id].process(left, right);
    }
}

float DelayEngine::getLeft()
{
    float left_out {0.0f};
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        if (!_voices[voice_id].getBypass()) {left_out += _voices[voice_id].getLeft();}
    }
    return left_out;
}

float DelayEngine::getRight()
{
    float right_out {0.0f};
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        if (!_voices[voice_id].getBypass()) {right_out += _voices[voice_id].getRight();}
    }
    return right_out;
}

void DelayEngine::setMasterDelayTime(float samples)
{
    // ensure samples are in range
    if (samples < 0.01f) {samples = 0.01f;}
    else if (samples >= _max_delay) {samples = _max_delay - 1;}
    _master_delay_time = samples;

    // set delay time per voice according to each voice's ratio
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        _voices[voice_id].setDelayTime(_master_delay_time * _ratios[voice_id]);
    }
}

void DelayEngine::setMasterFeedback(float feedback)
{
    // ensure feedback is in range
    _master_feedback = enforceRatio(feedback);

    // set feedback for each voice
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        _voices[voice_id].setFeedback(_master_feedback);
    }
}

void DelayEngine::setMasterFlutter(float flutter)
{
    // ensure flutter is in range
    _master_flutter = enforceRatio(flutter);

    // set flutter for each voice
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        _voices[voice_id].setFlutter(_master_flutter);
    }
}

void DelayEngine::setPingPongMode(bool b)
{
    for (int voice_id{0}; voice_id < _voice_count; voice_id++)
    {
        _voices[voice_id].setPingPongMode(b);
    }
}

void DelayEngine::setDelayRatio(int voice_id, float ratio)
{
    _ratios[voice_id] = enforceRatio(ratio);
}

void DelayEngine::setPan(int voice_id, float pan)
{
    pan = enforceRatio(pan);
    _voices[voice_id].setPan(pan);
}

float DelayEngine::enforceRatio(float x)
{
    x = std::max(0.0f, x);
    x = std::min(1.0f, x);
    return x;
}