#pragma once
#include <JuceHeader.h>
#include "Configuration/Samples.h"
#include "Synthesiser/PluginSynthesiser.h"
#include "PluginPresetManager.h"

class PluginAudioProcessor : public juce::AudioProcessor
{
public:

	PluginAudioProcessor();
	~PluginAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	void noteOnSynthesisers(int midiNoteNumber, float velocity);
	void noteOnSynthesisers(int midiNoteNumber, float velocity, std::string micId);

	std::vector<int> getMidiNotesVector();
	juce::AudioProcessorValueTreeState& getParameterValueTreeState() const;

	void loadAndPlayMidiFile(const juce::File& midiFile);

	PluginPresetManager& getPresetManager();
private:

	std::unique_ptr<juce::AudioFormatManager> mAudioFormatManager;
	std::unique_ptr<PluginPresetManager> mPresetManager;
	std::unique_ptr<juce::AudioProcessorValueTreeState> mParameterValueTreeState;

	std::vector<std::unique_ptr<PluginSynthesiser>> mSynthesiserPtrVector;
	std::vector<std::unique_ptr<juce::AudioBuffer<float>>> mInternalBufferPtrVector;

	std::vector<std::unique_ptr<juce::dsp::Compressor<float>>> mCompressors;

	std::vector<std::pair<double, juce::MidiMessage>> mScheduledMidiEvents;
	long long mCurrentSamplePosition = 0;

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessor)
};
