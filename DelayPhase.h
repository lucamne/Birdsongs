#pragma once

template <int MAX_DELAY, int VOICES>
class DelayPhase
{
public:
    DelayPhase()
    :_wptr{_dline},
    _feedback{0.25f},
    _time_spread{2.0f / 3.0f} 
    {
        for (int i{0}; i < VOICES; i++)
        {
            _rptr_arr[i] = 0.0f;
        }

        setDelayTime(MAX_DELAY - 1);
    }
    // adds new sample to delay line
    void process(float in)
    {
        // clear buffers
        _lbuff = 0.0f;
        _rbuff = 0.0f;

        // read next sample from delay line
        for (int i{0}; i < VOICES; i++)
        {
            // Get current delay time
            float curr_delay {static_cast<float>(_wptr - _dline) - _rptr_arr[i]};
            // enforce positive delay
            if (curr_delay <= 0.0f) {curr_delay += static_cast<float>(MAX_DELAY);}
            // get difference from expected delay
            const float delay_diff {curr_delay - _delay_times[i]};
            const float interp_amnt {delay_diff / static_cast<float>(MAX_DELAY)};

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
        if (s < static_cast<float>(VOICES)) {s = static_cast<float>(VOICES);}

        for (int i{0}; i < VOICES; i++)
        {
            _delay_times[i] = ratio * s;
            ratio *= _time_spread;
        }
    }

private:
    float _dline[MAX_DELAY]{};
    float _rptr_arr[VOICES]{};
    float* _wptr{};
    float _delay_times[VOICES]{};

    float _feedback{};
    float _time_spread{}; //> controls the variance of delay times

    float _lbuff{};
    float _rbuff{};
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
};