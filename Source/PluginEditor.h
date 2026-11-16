// Genre Bender - editor: vaporwave sunset look, part tabs, editable piano roll, drag-out MIDI.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace gbui
{
const juce::Colour kBg { 0xff120a1f }, kPanel { 0xff1c1030 }, kInk { 0xfff4eaff }, kMuted { 0xffa48fc4 },
                   kPink { 0xffff71ce }, kCyan { 0xff01cdfe }, kYellow { 0xfffffb96 }, kMint { 0xff05ffa1 },
                   kPurple { 0xffb967ff }, kOrange { 0xffff9f43 };
inline juce::Colour partColour(int p)
{
    static const juce::Colour c[6] = { kPink, kCyan, kYellow, kMint, kPurple, kOrange };
    return p >= 0 && p < 6 ? c[p] : kInk;
}

class VaporLook : public juce::LookAndFeel_V4
{
public:
    VaporLook();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getTextButtonFont(juce::TextButton&, int h) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
};

// A knob with its name underneath
class Knob : public juce::Component
{
public:
    Knob(const juce::String& name);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String name;
};

// Button you drag onto FL Studio to drop a MIDI file there
class DragMidiButton : public juce::Component
{
public:
    DragMidiButton(GenreBenderProcessor& p, juce::String text) : proc(p), label(std::move(text)) {}
    void setPart(int p) { part = p; repaint(); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
private:
    GenreBenderProcessor& proc;
    juce::String label;
    int part = -1;
    bool dragging = false;
};

class PianoRoll : public juce::Component, private juce::ScrollBar::Listener
{
public:
    explicit PianoRoll(GenreBenderProcessor&);
    void refresh();                              // re-read the song from the processor
    void setActivePart(int p) { activePart = p; repaint(); }
    void setSnap(double beats) { snap = beats; }
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void zoomToFit();
    void tick();                                 // playhead

private:
    void scrollBarMoved(juce::ScrollBar*, double) override;
    juce::Rectangle<int> gridArea() const;
    float xForBeat(double b) const;
    double beatForX(float x) const;
    float yForPitch(int p) const;
    int pitchForY(float y) const;
    int hitNote(juce::Point<float> pos, bool& onEdge) const;
    void commit();

    GenreBenderProcessor& proc;
    std::vector<gb::Note> notes[gb::NumParts];
    gb::Plan plan;
    double totalBeats = 32;
    int activePart = -1;
    double snap = 0.25;
    double pxPerBeat = 28;
    double scrollBeat = 0;
    int topPitch = 96;
    float rowH = 9.0f;
    double playBeat = -1;

    enum Drag { None, Move, Resize, Create };
    Drag drag = None;
    int dragIndex = -1;
    gb::Note dragOrig;
    juce::Point<float> dragStart;
    double lastLength = 1.0;
    bool needsFit = true;

    juce::ScrollBar hbar { false };
    static constexpr int kKeysW = 44, kRulerH = 26, kBarH = 12;
};

} // namespace gbui

class GenreBenderEditor : public juce::AudioProcessorEditor,
                          private juce::ChangeListener,
                          private juce::Timer
{
public:
    explicit GenreBenderEditor(GenreBenderProcessor&);
    ~GenreBenderEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void selectPart(int p);                      // -1 = ALL
    void updateModeVisibility();

    GenreBenderProcessor& proc;
    gbui::VaporLook look;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::ComboBox genre, key, tonality, mode, complexity;
    std::unique_ptr<CA> aGenre, aKey, aTon, aMode, aCx;
    gbui::Knob loopBars { "LOOP BARS" }, minutes { "SONG MINUTES" }, density { "DENSITY" }, swing { "SWING" }, humanize { "HUMANIZE" };
    std::unique_ptr<SA> aLoop, aMin, aDen, aSwing, aHum;
    juce::Slider songSeed;
    std::unique_ptr<SA> aSeed;
    juce::TextButton dice { "DICE" }, generate { "GENERATE" };

    juce::TextButton tabs[gb::NumParts + 1];
    int activePart = -1;
    juce::ComboBox partType;
    std::unique_ptr<CA> aType;
    juce::Label partSeedLabel;
    juce::TextButton reroll { "RE-ROLL" };
    juce::ToggleButton lock { "LOCK" }, mute { "MUTE" };
    std::unique_ptr<BA> aLock, aMute;
    gbui::DragMidiButton dragPart;
    juce::ComboBox snapBox;

    gbui::PianoRoll roll;

    juce::TextButton play { "PLAY" }, exportBtn { "EXPORT MIDI" }, fit { "FIT" };
    juce::ToggleButton previewSound { "PREVIEW SOUND" }, midiOut { "MIDI OUT" };
    std::unique_ptr<BA> aPrev, aMidi;
    gbui::Knob volume { "VOLUME" };
    std::unique_ptr<SA> aVol;
    gbui::DragMidiButton dragAll;
    juce::Label status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenreBenderEditor)
};
