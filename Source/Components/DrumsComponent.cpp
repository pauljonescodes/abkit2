#include "DrumsComponent.h"
#include "../Configuration/Samples.h"
#include "../Configuration/GeneralMidi.h"
#include "../Configuration/Strings.h"


DrumsComponent::DrumsComponent(
	const std::vector<int> midiNotesVector, 
	juce::AudioProcessorValueTreeState& apvts,
	PluginPresetManager& presetManager)
{
	mPresetComponent = std::make_unique<PresetComponent>(presetManager);
	addAndMakeVisible(mPresetComponent.get());

	mVelocitySlider = std::make_unique<juce::Slider>();
	mVelocitySlider->setRange(0, 127, 1);
	mVelocitySlider->setValue(100);
	addAndMakeVisible(mVelocitySlider.get());

	mVelocityLabel = std::make_unique<juce::Label>();
	mVelocityLabel->setText(strings::velocity, juce::dontSendNotification);
	mVelocityLabel->attachToComponent(mVelocitySlider.get(), true);
	addAndMakeVisible(mVelocityLabel.get());

	for (int note : midiNotesVector)
	{
		juce::TextButton* button = new juce::TextButton(generalmidi::midiNoteToNameMap.at(note) + "\n" + juce::String(note).toStdString() + " " + juce::MidiMessage::getMidiNoteName(note, true, true, 4).toStdString());
		button->setComponentID(juce::String(note));
		mMidiNoteButtons.add(button);
		addAndMakeVisible(button);
		button->addListener(this);
	}

	mMidiFileButton = std::make_unique <juce::TextButton>("MIDI");
	mMidiFileButton->setComponentID(juce::String("midi_file"));
	addAndMakeVisible(mMidiFileButton.get());
	mMidiFileButton->addListener(this);

	mFileChooser = std::make_unique<juce::FileChooser>("Please select the midi you want to load...",
		juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
		"*.mid");

	auto* multiOutParameter = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters::multiOutId));
	mMultiOutToggleButton.reset(new juce::ToggleButton(strings::multiOut));
	addAndMakeVisible(mMultiOutToggleButton.get());
	mMultiOutToggleButton->setToggleState(multiOutParameter->get(), juce::dontSendNotification);
	mMultiOutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
		apvts,
		parameters::multiOutId,
		*mMultiOutToggleButton
		);

	resized();
}

DrumsComponent::~DrumsComponent()
{
	for (auto button : mMidiNoteButtons) {
		button = nullptr;
	}
}


void DrumsComponent::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void DrumsComponent::resized()
{
	const auto localBounds = getLocalBounds();
	auto presetControls = localBounds;
	auto sampleControls = localBounds;

	presetControls.setHeight(50);
	sampleControls.removeFromTop(50);
	sampleControls.setHeight(50);

	mMidiFileButton->setBounds(sampleControls.removeFromRight(localBounds.proportionOfWidth(0.1)));
	mMultiOutToggleButton->setBounds(sampleControls.removeFromLeft(localBounds.proportionOfWidth(0.1)));
	mVelocitySlider->setBounds(sampleControls.withTrimmedLeft(localBounds.proportionOfWidth(0.1)));

	mPresetComponent->setBounds(presetControls);

	// Layout for the MIDI note buttons
	int numRows = std::max(1, (localBounds.getHeight()) / 125);
	int numCols = std::max(1, localBounds.getWidth() / 125);
	int buttonWidth = localBounds.getWidth() / numCols;
	int buttonHeight = localBounds.getHeight() / numRows;

	for (int i = 0; i < mMidiNoteButtons.size(); ++i)
	{
		int row = i / numCols;
		int col = i % numCols;
		mMidiNoteButtons[i]->setBounds(col * buttonWidth, row * buttonHeight + 100, buttonWidth, buttonHeight);
	}
}

void DrumsComponent::buttonClicked(juce::Button* button)
{
	const juce::String componentID = button->getComponentID();

	if (mMidiFileButton->getComponentID() == componentID) {
		auto folderChooserFlags = juce::FileBrowserComponent::openMode;
		mFileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
			{
				juce::File midiFile(chooser.getResult());
		if (mOnMidiFileChoser.has_value()) {
			mOnMidiFileChoser.value()(midiFile);
		}

			});
	}
	else {
		int midiNoteValue = componentID.getIntValue();
		float velocity = mVelocitySlider->getValue() / 127.0f;

		if (mOnDrumMidiButtonClicked.has_value()) {
			mOnDrumMidiButtonClicked.value()(midiNoteValue, velocity);
		}
	}
}

