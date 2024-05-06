#include "DelayPhase.h"

#include "daisy_seed.h"

///*** Global Values ***///
static constexpr int SAMPLE_RATE{48000};

daisy::DaisySeed hw{}; //> Daisy seed hardware object
daisy::CpuLoadMeter load_meter{};

// init effects
DelayPhase<SAMPLE_RATE,SAMPLE_RATE> delay{};
float delay_mix{0.5f};

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
	hw.StartAudio(AudioCallback);
	hw.StartLog();
	hw.PrintLine("Hardware Initialized");
	const float hw_sample_rate {hw.AudioSampleRate()};

	// init load meter
	load_meter.Init(hw_sample_rate,hw.AudioBlockSize());
	
	// init effects 
	delay.init();
	// add voices
	delay.addVoice();
	delay.addVoice();
	delay.addVoice();
	// change params for delay voice 2
	delay.setRatio(1,0.625f);
	delay.setPan(1,1.0f);
	delay.setRatio(2,0.37f);
	delay.setPan(2,0.0f);
	hw.PrintLine("Effects Initialized");

	// Configure temp button for adding new voices
	// daisy::GPIO new_voice_button{};
	// new_voice_button.Init(daisy::seed::D18, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	// bool button_state{false};
	// int voice_count{1};

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
		// // add voice if button is pressed
		// {
		// 	bool temp{new_voice_button.Read()};
		// 	if (!button_state && temp)
		// 	{
		// 		chorus.addVoice();
		// 		voice_count++;
		// 	}
		// 	button_state = temp;
		// }

		// of delay time pot has moved significantly
		if (std::abs(hw.adc.GetFloat(0) - delay_time_pot) >= 0.01f)
		{
			delay_time_pot = hw.adc.GetFloat(0);
			if (delay_time_pot > 0.99f) {delay_time_pot = 1.0f;}
			else if (delay_time_pot < 0.01f) {delay_time_pot = 0.0f;}

			delay.setMasterDelay(delay_time_pot);
		}

		// of delay time pot has moved significantly
		if (std::abs(hw.adc.GetFloat(1) - feedback_pot) >= 0.01f)
		{
			feedback_pot = hw.adc.GetFloat(1);
			if (feedback_pot > 0.99f) {feedback_pot = 1.0f;}
			else if (feedback_pot < 0.01f) {feedback_pot = 0.0f;}
			delay.setFeedback(feedback_pot);
		}

		if (std::abs(hw.adc.GetFloat(2) - mix_pot) >= 0.01f)
		{
			mix_pot = hw.adc.GetFloat(2);
			if (mix_pot > 0.99f) {mix_pot = 1.0f;}
			else if (mix_pot < 0.01f) {mix_pot = 0.0f;}
			delay_mix = mix_pot;
		}

		// Print to serial monitor
		hw.PrintLine("Load:%f",load_meter.GetAvgCpuLoad());
	}
}
