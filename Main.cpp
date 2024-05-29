#include "DelayEngine.h"
#include "Encoder.h"
#include "Potentiometer.h"

//#include "lcd_hd44780.h"
#include "daisy_seed.h"

#include <map>

///*** Global Values ***///
static constexpr int SAMPLE_RATE{48000};
/// constants for delay
static constexpr int DELAY_VOICES{3};
static constexpr int MAX_DELAY{SAMPLE_RATE * 2};
float DSY_SDRAM_BSS DELAY_LEFT_BUFFER[MAX_DELAY * DELAY_VOICES];
float DSY_SDRAM_BSS DELAY_RIGHT_BUFFER[MAX_DELAY * DELAY_VOICES];

/// constants for chorus
static constexpr int CHORUS_VOICES{2};
static constexpr int MAX_CHORUS_DELAY{SAMPLE_RATE / 50};
// multiply by 2 because each delay voice needs two delay lines
float DSY_SDRAM_BSS CHORUS_LEFT_BUFFER[MAX_CHORUS_DELAY * CHORUS_VOICES];
float DSY_SDRAM_BSS CHORUS_RIGHT_BUFFER[MAX_CHORUS_DELAY * CHORUS_VOICES];

daisy::DaisySeed hw{}; //> Daisy seed hardware object
daisy::CpuLoadMeter load_meter{};

// init effects
DelayEngine delay{};
DelayEngine chorus{};
float delay_mix{0.0f};
static constexpr float chorus_mix{1.0};
static bool chorus_on{false};

void AudioCallback(daisy::AudioHandle::InterleavingInputBuffer in, daisy::AudioHandle::InterleavingOutputBuffer out, size_t size)
{
	load_meter.OnBlockStart();
    for (size_t i = 0; i < size; i+=2)
    {
		float left_out{in[i] * 0.5f};
		float right_out{in[i] * 0.5f};

		// delay stage
		delay.process(left_out, right_out);
        left_out = delay.getLeft() * delay_mix + left_out * (1 - delay_mix);
        right_out = delay.getRight() * delay_mix + right_out * (1 - delay_mix);

		// chorus stage
		if (chorus_on)
		{
			chorus.process(left_out, right_out);
			left_out = chorus.getLeft() * chorus_mix + left_out * (1 - chorus_mix);
			right_out = chorus.getRight() * chorus_mix + right_out * (1 - chorus_mix);
		}

		// passthrough
		// out[i] = in[i];
		// out[i+1] = in[i];

		out[i] = left_out;
		out[i+1] = right_out;
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
	
	/// init delay 
	delay.init(DELAY_LEFT_BUFFER, DELAY_RIGHT_BUFFER, MAX_DELAY, DELAY_VOICES, SAMPLE_RATE);
	// set pans of voices
	delay.setPan(0,0.0f);
	delay.setPan(1,0.5f);
	delay.setPan(2,1.0f);
	// ratios are hardcoded temporarily, will be assigned to pots
	delay.setDelayRatio(0,0.67f);
	delay.setDelayRatio(1,1.0f);
	delay.setDelayRatio(2,0.44f);

	/// init chorus
	chorus.init(CHORUS_LEFT_BUFFER, CHORUS_RIGHT_BUFFER, MAX_CHORUS_DELAY, CHORUS_VOICES, SAMPLE_RATE);
	// set voice panning
	chorus.setPan(0,0.0f);
	chorus.setPan(1,1.0f);
	// add slight variation in delay time
	chorus.setDelayRatio(0,1.0f);
	chorus.setDelayRatio(1,0.2f);
	chorus.setMasterFlutter(0.35f);
	chorus.setDetune(0,-300.0f);
	chorus.setMasterFeedback(0.13f);
	chorus.setMasterDelayTime(MAX_CHORUS_DELAY);

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
	daisy::GPIO ping_pong_switch{};
	ping_pong_switch.Init(daisy::seed::D4, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	daisy::GPIO chorus_switch{};
	chorus_switch.Init(daisy::seed::D5, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	


	// /// init lcd screen
	// daisy::GPIO lcd_rs{};
	// daisy::LcdHD44780 lcd{};
	// daisy::LcdHD44780::Config lcd_config {	false,
	// 										false,
	// 										}

	///*** Pogram Loop ***///
	while(1)
	{
		delay.setBypass(0,voice1_switch.Read());
		delay.setBypass(1,voice2_switch.Read());
		delay.setBypass(2,voice3_switch.Read());
		delay.setPingPongMode(!ping_pong_switch.Read());
		chorus_on = !chorus_switch.Read();

		hw.PrintLine("Time:%f Feedback:%f Flutter:%f Mix:%f",
						(delay.getMasterDelayTime() / static_cast<float>(SAMPLE_RATE)) * 1000.0f, 
						delay.getMasterFeedback(),
						delay.getMasterFlutter(),
						delay_mix);
		// process pots
		if (pot_map[TIME].process())
		{
			delay.setMasterDelayTime(pot_map[TIME].getVal() * MAX_DELAY);
		}
		if (pot_map[FEEDBACK].process())
		{
			delay.setMasterFeedback(pot_map[FEEDBACK].getVal());
		}
		if (pot_map[FLUTTER].process())
		{
			delay.setMasterFlutter(pot_map[FLUTTER].getVal());
		}
		if (pot_map[MIX].process())
		{
			delay_mix = pot_map[MIX].getVal();
		}

		// print to lcd screen
	}
}