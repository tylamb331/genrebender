// Genre Bender - plugin processor. Holds the song, plays it in time with the host (or its own
// Preview clock), sends MIDI out on channels 1-6 (one per part) and voices it with the preview synth.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "Generator.h"
#include "PreviewSynth.h"

class GenreBenderProcessor : public juce::AudioProcessor,
                             public juce::ChangeBroadcaster,
                             private juce::AudioProcessorValueTreeState::Listener,
                             private juce::AsyncUpdater
{
public:
    GenreBenderProcessor();
    ~GenreBenderProcessor() override;

    // --- the song (message thread) ---
    void generateAll();                         // re-rolls every part that isn't locked
    void rerollPart(int part);                  // new part seed for one part, regenerate just it
    void newSongSeed();                         // dice: new song seed, then generateAll
    std::vector<gb::Note> getPartNotes(int part) const;
    void setPartNotes(int part, std::vector<gb::Note> notes);   // piano roll edits
    const gb::Plan& getPlan() const { return plan; }
    double getTotalBeats() const { return plan.totalBars() * 4.0; }
    gb::Settings currentSettings() const;       // from the knobs
    juce::String describe() const;              // "City Pop - E minor - song 7"
    bool writeMidiFile(const juce::File& file, int partOrAll) const;   // -1 = all parts
    juce::File tempMidiFile(int partOrAll) const;

    // --- transport info for the editor ---
    double getPlayBeat() const { return playBeat.load(); }
    bool isPreviewPlaying() const { return previewPlaying.load(); }
    void setPreviewPlaying(bool on);
    double getBpm() const { return bpm.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String partParam(const char* what, int part) { return juce::String(what) + juce::String(part); }

    // --- AudioProcessor ---
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Genre Bender"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    struct Ev { double beat; int pitch; float vel; int part; bool on; };

    void parameterChanged(const juce::String& id, float) override;
    void handleAsyncUpdate() override;
    void rebuildPlayback();                     // notes -> sorted event list (message thread)
    void regenerate(const gb::Settings& s, bool respectLocks, int onlyPart);
    void sendAllOff(juce::MidiBuffer& midi, int sampleOffset);
    void emit(const Ev& e, juce::MidiBuffer& midi, int offset, bool midiOut);

    gb::Plan plan;
    gb::Settings genSettings;                   // the settings the current song was made with
    std::vector<gb::Note> notes[gb::NumParts];

    juce::SpinLock playLock;                    // guards events (audio thread only try-locks)
    std::vector<Ev> events;
    double loopBeats = 0;

    gb::PreviewSynth synth;
    bool active[gb::NumParts][128] = {};
    double sr = 44100.0;
    double lastEnd = -1.0;                      // beat where the previous block ended
    double internalBeat = 0.0;
    std::atomic<double> playBeat { -1.0 };
    std::atomic<double> bpm { 90.0 };
    std::atomic<bool> previewPlaying { false };
    std::atomic<bool> wantAllOff { false };
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenreBenderProcessor)
};
