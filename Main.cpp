#include "DelayVoice.h"
#include "Encoder.h"
#include "Potentiometer.h"

#include "daisy_seed.h"

#include <map>

///*** Global Values ***///
static constexpr int SAMPLE_RATE{48000};
static constexpr int MAX_DELAY{SAMPLE_RATE * 2};
float DSY_SDRAM_BSS SDRAM_BUFFER[MAX_DELAY * 2];

daisy::DaisySeed hw{}; //> Daisy seed hardware object
daisy::CpuLoadMeter load_meter{};

// init effects
DelayVoice delay{};
float delay_mix{0.3f};

void AudioCallback(daisy::AudioHandle::InterleavingInputBuffer in, daisy::AudioHandle::InterleavingOutputBuffer out, size_t size)
{
	load_meter.OnBlockStart();
    for (size_t i = 0; i < size; i+=2)
    {
		delay.process(in[i]);
        out[i] = delay.getLeft() * delay_mix + in[i] * (1 - delay_mix);
        out[i+1] = delay.getRight() * delay_mix + in[i] * (1 - delay_mix);
		// out[i] = in[i];
		// out[i+1] = in[i];
    }
	load_meter.OnBlockEnd();
}

int main(void)
{
	///*** Init section ***///
	// init hardware
	hw.Init();
	hw.SetAudioBlockSize(4); //> number of samples handled per callback
	hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartLog();
	const float hw_sample_rate {hw.AudioSampleRate()};

	// init load meter
	load_meter.Init(hw_sample_rate,hw.AudioBlockSize());
	
	// init effects 
	delay.init(SDRAM_BUFFER, MAX_DELAY, SAMPLE_RATE);

	hw.StartAudio(AudioCallback);

	// Configure ADC channel
	constexpr int NUM_POTS{4};
	enum AdcChannel {
		TIME = 0,
		FEEDBACK,
		FLUTTER,
		MIX
	};
	daisy::AdcChannelConfig adc_channel_config[NUM_POTS]{};
	// init adc ports
	adc_channel_config[TIME].InitSingle(daisy::seed::A11);
	adc_channel_config[FEEDBACK].InitSingle(daisy::seed::A10);
	adc_channel_config[FLUTTER].InitSingle(daisy::seed::A9);
	adc_channel_config[MIX].InitSingle(daisy::seed::A8);
	hw.adc.Init(adc_channel_config,NUM_POTS);	//> give handle to adc_cahnnel_config array and length
	hw.adc.Start(); //> start adc
	// Init Potentiometers (all pots are currently reversed due to prototype wiring error that is better solved in software)
	std::map<AdcChannel,Potentiometer> pot_map{};
	pot_map[TIME].init(&hw,TIME,true);
	pot_map[FEEDBACK].init(&hw,FEEDBACK,true);
	pot_map[FLUTTER].init(&hw,FLUTTER,true);
	pot_map[MIX].init(&hw,MIX,true);

	/// init switches
	daisy::GPIO voice1_switch{};
	voice1_switch.Init(daisy::seed::D1, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	daisy::GPIO voice2_switch{};
	voice2_switch.Init(daisy::seed::D2, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	daisy::GPIO voice3_switch{};
	voice3_switch.Init(daisy::seed::D3, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);

	///*** Pogram Loop ***///
	while(1)
	{
		hw.PrintLine("Time:%f Feedback:%f Flutter:%f Mix:%f",
						(delay.getDelayTime() / static_cast<float>(SAMPLE_RATE)) * 1000.0f, 
						delay.getFeedback(),
						delay.getFlutter(),
						delay_mix);
		// process pots
		if (pot_map[TIME].process())
		{
			delay.setDelayTime(pot_map[TIME].getVal() * MAX_DELAY);
		}
		if (pot_map[FEEDBACK].process())
		{
			delay.setFeedback(pot_map[FEEDBACK].getVal());
		}
		if (pot_map[FLUTTER].process())
		{
			delay.setFlutter(pot_map[FLUTTER].getVal());
		}
		if (pot_map[MIX].process())
		{
			delay_mix = pot_map[MIX].getVal();
		}
	}
}