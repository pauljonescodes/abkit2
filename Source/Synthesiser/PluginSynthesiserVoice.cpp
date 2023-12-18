#include <JuceHeader.h>
#include "PluginSynthesiserVoice.h"
#include "PluginSynthesiserSound.h"

PluginSynthesiserVoice::PluginSynthesiserVoice(
    juce::RangedAudioParameter& gainParameter,
    juce::RangedAudioParameter& panParameter,
    juce::AudioParameterBool& phaseParameter
) :
    mGainParameter(gainParameter), 
    mPanParameter(panParameter),
    mInvertPhaseParameter(phaseParameter)
{ }

PluginSynthesiserVoice::~PluginSynthesiserVoice() {}

bool PluginSynthesiserVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<const PluginSynthesiserSound*> (sound) != nullptr;
}

void PluginSynthesiserVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* s, int /*currentPitchWheelPosition*/)
{
    if (auto* sound = dynamic_cast<const PluginSynthesiserSound*> (s))
    {
        mSourceSamplePosition = 0.0;
        
        mVelocityGain = velocity;
        
        mAdsr.setSampleRate(sound->mSourceSampleRate);
        mAdsr.setParameters(sound->mAdsrParameters);
        mAdsr.noteOn();
    }
    else
    {
        jassertfalse; // this object can only play SamplerSounds!
    }
}

void PluginSynthesiserVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        mAdsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        mAdsr.reset();
    }
}

void PluginSynthesiserVoice::pitchWheelMoved(int newValue) {}
void PluginSynthesiserVoice::controllerMoved(int controllerNumber, int newValue) {}


void PluginSynthesiserVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (auto* playingSound = static_cast<PluginSynthesiserSound*> (getCurrentlyPlayingSound().get()))
    {
        auto& data = *playingSound->mData;
        const float* const inL = data.getReadPointer(0);
        const float* const inR = data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;
        
        float* outL = outputBuffer.getWritePointer(0, startSample);
        float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;
        
        while (--numSamples >= 0)
        {
            auto pos = (int)mSourceSamplePosition;
            auto alpha = (float)(mSourceSamplePosition - pos);
            auto invAlpha = 1.0f - alpha;
            
            float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
            float r = (inR != nullptr) ? (inR[pos] * invAlpha + inR[pos + 1] * alpha) : l;
            
            auto envelopeValue = mAdsr.getNextSample();
            
            float panValue = mPanParameter.getValue();
            float panNormalisedValue = constants::panNormalizableRange.convertFrom0to1(panValue);
            float panLeft = panNormalisedValue <= 0.0f ? 1.0f : 1.0f - panNormalisedValue;
            float panRight = panNormalisedValue >= 0.0f ? 1.0f : 1.0f + panNormalisedValue;
            
            float phaseMultiplier = mInvertPhaseParameter.get() ? -1 : 1;
            float normalizedValue = mGainParameter.getValue(); // Get the normalized value

            float dB = constants::gainNormalizableRange.convertFrom0to1(normalizedValue);

            // Convert dB to linear gain
            float linearGain = std::pow(10.0f, dB / 20.0f);
            
            l *= panLeft * envelopeValue * linearGain * mVelocityGain * phaseMultiplier;
            r *= panRight * envelopeValue * linearGain * mVelocityGain * phaseMultiplier;
            
            if (outR != nullptr)
            {
                *outL++ += l;
                *outR++ += r;
            }
            else
            {
                *outL++ += (l + r) * 0.5f;
            }
            
            mSourceSamplePosition += 1.0f;
            
            if (mSourceSamplePosition > playingSound->mLength)
            {
                stopNote(0.0f, false);
                break;
            }
        }
    }
}