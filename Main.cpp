#include "DelayPhase.h"

#include "daisy_seed.h"

///*** Global Values ***///
static constexpr int SAMPLE_RATE{48000};

daisy::DaisySeed hw{}; //> Daisy seed hardware object
daisy::CpuLoadMeter load_meter{};

// init effects
DelayPhase<SAMPLE_RATE,SAMPLE_RATE> chorus{};
float chorus_mix{0.5f};

void AudioCallback(daisy::AudioHandle::InterleavingInputBuffer in, daisy::AudioHandle::InterleavingOutputBuffer out, size_t size)
{
	load_meter.OnBlockStart();
    for (size_t i = 0; i < size; i+=2)
    {
		chorus.process(in[i]);
        out[i] = chorus.getLeft() * chorus_mix + in[i] * (1 - chorus_mix);
        out[i+1] = chorus.getRight() * chorus_mix + in[i] * (1 - chorus_mix);
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
	hw.StartAudio(AudioCallback);
	hw.StartLog();
	hw.PrintLine("Hardware Initialized");
	const float hw_sample_rate {hw.AudioSampleRate()};

	chorus.addVoice();

	// init load meter
	load_meter.Init(hw_sample_rate,hw.AudioBlockSize());
	
	// init effects 
	hw.PrintLine("Effects Initialized");

	// Configure ADC input
	constexpr std::size_t NUM_POTS{3};
	daisy::AdcChannelConfig adc_channel_config[NUM_POTS]{};
	adc_channel_config[0].InitSingle(daisy::seed::A0);
	adc_channel_config[1].InitSingle(daisy::seed::A1);
	adc_channel_config[2].InitSingle(daisy::seed::A2);
	hw.adc.Init(adc_channel_config,3);	//> give handle to adc_cahnnel_config array and length
	hw.adc.Start(); //> start adc
	hw.PrintLine("External Hardware Initialized");

	float delay_time_pot{hw.adc.GetFloat(0)};
	float feedback_pot{hw.adc.GetFloat(1)};
	float mix_pot{hw.adc.GetFloat(2)};

	///*** Pogram Loop ***///
	while(1)
	{
		// of delay time pot has moved significantly
		if (std::abs(hw.adc.GetFloat(0) - delay_time_pot) >= 0.01f)
		{
			delay_time_pot = hw.adc.GetFloat(0);
			if (delay_time_pot > 0.99f) {delay_time_pot = 1.0f;}
			else if (delay_time_pot < 0.01f) {delay_time_pot = 0.0f;}

			chorus.setMasterDelay(delay_time_pot);
		}

		// of delay time pot has moved significantly
		if (std::abs(hw.adc.GetFloat(1) - feedback_pot) >= 0.01f)
		{
			feedback_pot = hw.adc.GetFloat(1);
			if (feedback_pot > 0.99f) {feedback_pot = 1.0f;}
			else if (feedback_pot < 0.01f) {feedback_pot = 0.0f;}

			chorus.setFeedback(feedback_pot);
		}
		// Print to serial monitor

		if (std::abs(hw.adc.GetFloat(2) - mix_pot) >= 0.01f)
		{
			mix_pot = hw.adc.GetFloat(2);
			if (mix_pot > 0.99f) {mix_pot = 1.0f;}
			else if (mix_pot < 0.01f) {mix_pot = 0.0f;}

			chorus_mix = mix_pot;
		}
		hw.PrintLine("Load:%f",load_meter.GetAvgCpuLoad());
	}
}
