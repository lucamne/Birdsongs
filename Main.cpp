#include "DelayPhase.h"
#include "Encoder.h"
#include "Potentiometer.h"

#include "daisy_seed.h"

///*** Global Values ***///
static constexpr int SAMPLE_RATE{48000};
static constexpr int MAX_DELAY{SAMPLE_RATE * 2};
float DSY_SDRAM_BSS SDRAM_BUFFER[MAX_DELAY * 50];

daisy::DaisySeed hw{}; //> Daisy seed hardware object
daisy::CpuLoadMeter load_meter{};

// init effects
DelayPhase<MAX_DELAY,SAMPLE_RATE> delay{};
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

void processInput(daisy::GPIO* button, Encoder* encoder, Potentiometer* pot_arr);

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
	delay.init(SDRAM_BUFFER, SAMPLE_RATE * 50);
	// add voices
	delay.addVoice();
	hw.PrintLine("Effects Initialized");

	// Configure main button
	daisy::GPIO buttons[3]{};
	buttons[0].Init(daisy::seed::D18, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	// add new voice button
	daisy::GPIO add_voice_button{};
	buttons[1].Init(daisy::seed::D22, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	// add delete voice button
	daisy::GPIO delete_voice_button{};
	buttons[2].Init(daisy::seed::D23, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);

	// Configure encoder
	daisy::GPIO encoder_out_a{};
	daisy::GPIO encoder_out_b{};
	daisy::GPIO sw{};
	encoder_out_a.Init(daisy::seed::D20, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLDOWN);
	encoder_out_b.Init(daisy::seed::D21, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLDOWN);
	sw.Init(daisy::seed::D19, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
	Encoder encoder{};
	encoder.init(&encoder_out_a,&encoder_out_b,&sw);

	// Configure ADC ports
	constexpr int NUM_POTS{3};
	daisy::AdcChannelConfig adc_channel_config[NUM_POTS]{};
	adc_channel_config[0].InitSingle(daisy::seed::A0);
	adc_channel_config[1].InitSingle(daisy::seed::A1);
	adc_channel_config[2].InitSingle(daisy::seed::A2);
	hw.adc.Init(adc_channel_config,NUM_POTS);	//> give handle to adc_cahnnel_config array and length
	hw.adc.Start(); //> start adc
	// Init Potentiometers
	Potentiometer pot_arr[NUM_POTS]{};
	for (int i{0}; i < NUM_POTS; i++) {pot_arr[i].init(&hw,i);}
	
	hw.PrintLine("External Hardware Initialized");


	///*** Pogram Loop ***///
	while(1)
	{
		processInput(buttons,&encoder,pot_arr);
	}
}

// process inputs, MAKE SURE pot_arr has approriate size
void processInput(daisy::GPIO* buttons, Encoder* encoder, Potentiometer* pot_arr)
{
	static bool main_button_state{!buttons[0].Read()};
	static bool add_voice_button{!buttons[1].Read()};
	static bool delete_voice_button{!buttons[2].Read()};
	static int current_voice{0};

	// handle voice selection
	current_voice += encoder->process();
	if (current_voice < 0) {current_voice = 0;}
	// add two to voice count for add and delete screens
	else if (current_voice >= delay.getVoiceCount()) {current_voice = delay.getVoiceCount() - 1;}
	
	// get button state
	main_button_state = !buttons[0].Read();
	
	// add new voice

	bool temp_button{!buttons[1].Read()};
	if (add_voice_button && !temp_button)
	{
		delay.addVoice();
		current_voice = delay.getVoiceCount();
	}
	add_voice_button = temp_button;
	// delete current voice
	temp_button = !buttons[2].Read();
	if (delete_voice_button && !temp_button && delay.getVoiceCount() > 1)
	{
		delay.deleteVoice(current_voice);
		current_voice = std::max(current_voice - 1, 0);
	}
	delete_voice_button = temp_button;

	// if not pressed show main controls
	if (!main_button_state) 
	{
		hw.PrintLine("Voice:%d Time:%f Warble:%f Mix:%f",
						current_voice + 1, 
						delay.getMasterDelayTimeInMS(), 
						delay.getFlutter(),
						delay_mix);


		if (pot_arr[0].process())
		{
			delay.setMasterDelay(pot_arr[0].getVal());
		}
		if (pot_arr[1].process())
		{
			delay.setFlutter(pot_arr[1].getVal());
		}
		if (pot_arr[2].process())
		{
			delay_mix = pot_arr[2].getVal();
		}
	}
	// else show voice specific controls
	else 
	{
		hw.PrintLine("Voice:%d Ratio:%f Feedback:%f Pan:%f",
						current_voice + 1, 
						delay.getRatio(current_voice), 
						delay.getFeedback(current_voice), 
						delay.getPan(current_voice));

		if (pot_arr[0].process())
		{
			delay.setRatio(current_voice ,pot_arr[0].getVal());
		}
		if (pot_arr[1].process())
		{
			delay.setFeedback(current_voice, pot_arr[1].getVal());
		}
		if (pot_arr[2].process())
		{
			delay.setPan(current_voice, pot_arr[2].getVal());
		}
	}
	
}