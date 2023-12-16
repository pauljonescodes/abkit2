/*
 ==============================================================================

 CLROHSynthesiser.cpp
 Created: 8 Dec 2023 5:21:07pm
 Author:  paulm

 ==============================================================================
 */

#include "PluginSynthesiser.h"
#include "./Configuration/Samples.h"
#include "PluginSamplerVoice.h"

PluginSynthesiser::PluginSynthesiser()
{
	setNoteStealingEnabled(false);
}

void PluginSynthesiser::noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) {

}

void PluginSynthesiser::noteOn(const int midiChannel, const int midiNoteNumber, const float velocity)
{
	if (mMidiNoteToInstruments.find(midiNoteNumber) != mMidiNoteToInstruments.end())
	{
		auto& instrument = mMidiNoteToInstruments.at(midiNoteNumber);

		for (auto& voice : voices)
		{
			for (int stopsMidiNote : instrument.stopsMidiNotes)
			{
				auto currentlyPlayingSounds = voice->getCurrentlyPlayingSound();
				if (currentlyPlayingSounds != nullptr && currentlyPlayingSounds->appliesToNote(stopsMidiNote))
				{
					voice->stopNote(1, true);
				}
			}
		}

		auto& intensities = instrument.velocities;
		float position = velocity * (intensities.size() - 1);

		int lowerIntensityIndex = static_cast<int>(position);
		int higherIntensityIndex = lowerIntensityIndex + 1;

		if (lowerIntensityIndex >= intensities.size())
		{
			lowerIntensityIndex = 0;
		}

		if (higherIntensityIndex >= intensities.size() || velocity == 0)
		{
			higherIntensityIndex = lowerIntensityIndex;
		}

		float blendRatio = position - lowerIntensityIndex;
		float perceivedBlendRatio = lowerIntensityIndex != higherIntensityIndex ? std::pow(blendRatio, 0.33) : 1.0f;

		auto& lowerIntensity = intensities[lowerIntensityIndex];
		auto samplesSize = lowerIntensity.variations.size();

		if (lowerIntensity.currentVariationIndex < samplesSize)
		{
			auto& variation = lowerIntensity.variations[lowerIntensity.currentVariationIndex];

			for (auto& microphone : variation.microphones)
			{
				auto& sound = microphone.sound;
				auto& voice = microphone.voice;

				if (sound->appliesToNote(midiNoteNumber) && sound->appliesToChannel(midiChannel))
				{
					stopVoice(voice.get(), 1.0f, true);
					startVoice(voice.get(), sound.get(), midiChannel, midiNoteNumber, perceivedBlendRatio);
				}
			}

			lowerIntensity.currentVariationIndex = (lowerIntensity.currentVariationIndex + 1) % samplesSize;
		}

		if (lowerIntensityIndex != higherIntensityIndex)
		{
			auto& higherIntensity = intensities[higherIntensityIndex];
			auto& variation = higherIntensity.variations[higherIntensity.currentVariationIndex];

			for (auto& microphone : variation.microphones) {
				auto& sound = microphone.sound;
				auto& voice = microphone.voice;

				if (sound->appliesToNote(midiNoteNumber) && sound->appliesToChannel(midiChannel))
				{
					stopVoice(voice.get(), 1.0f, true);
					startVoice(voice.get(), sound.get(), midiChannel, midiNoteNumber, perceivedBlendRatio);
				}
			}

			higherIntensity.currentVariationIndex = (higherIntensity.currentVariationIndex + 1) % samplesSize;
		}
	}
}

void PluginSynthesiser::addSample(
	const std::string resourceName,
	const int bitRate,
	const int bitDepth,
	const int midiNote,
	const std::vector<int> stopsMidiNotes,
	const int velocityIndex,
	const int variationIndex,
	juce::AudioFormatManager& audioFormatManager,
	juce::RangedAudioParameter& gainParameter,
	juce::RangedAudioParameter& panParameter,
	juce::AudioParameterBool& phaseParameter
) {
	juce::BigInteger range;
	range.setRange(midiNote, 1, true);
	int dataSizeInBytes;

	const char* sourceData = BinaryData::getNamedResource(resourceName.c_str(), dataSizeInBytes);
	auto memoryInputStream = std::make_unique<juce::MemoryInputStream>(sourceData, dataSizeInBytes, false);
	juce::AudioFormatReader* reader = audioFormatManager.createReaderFor(std::move(memoryInputStream));

	double maxSampleLengthSeconds = dataSizeInBytes / (samples::bitRate * (samples::bitDepth / 8.0));
	PluginSamplerSound* sound = new PluginSamplerSound(juce::String(resourceName), *reader, range, midiNote, 0.0, 0.0, maxSampleLengthSeconds);

	addSound(sound);

	auto& instrument = mMidiNoteToInstruments[midiNote];

	instrument.stopsMidiNotes = stopsMidiNotes;

	while (instrument.velocities.empty() || velocityIndex >= instrument.velocities.size())
	{
		instrument.velocities.emplace_back();
	}

	auto& velocity = instrument.velocities[velocityIndex];
	while (velocity.variations.empty() || variationIndex >= velocity.variations.size())
	{
		velocity.variations.emplace_back();
	}

	auto microphone = Microphone(PluginSamplerSound::Ptr(sound),
		std::make_unique<PluginSamplerVoice>(
			gainParameter,
			panParameter,
			phaseParameter)
	);

	addVoice(microphone.voice.get());
	velocity.variations[variationIndex].microphones.push_back(std::move(microphone));
}

std::vector<int> PluginSynthesiser::getMidiNotesVector()
{
	std::vector<int> keys;
	for (const auto& pair : mMidiNoteToInstruments) {
		keys.push_back(pair.first); // Collect the keys
	}
	return keys;
}