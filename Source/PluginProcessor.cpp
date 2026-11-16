// Genre Bender - plugin processor. See PluginProcessor.h.
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gb;

namespace
{
const char* kTonality[] = { "Major", "Minor" };

juce::StringArray toArray(const std::vector<std::string>& v)
{
    juce::StringArray a;
    for (auto& s : v) a.add(s);
    return a;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout GenreBenderProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray genres, keys;
    for (int i = 0; i < kNumGenres; ++i) genres.add(kGenres[i]);
    for (auto* k : kKeyNames) keys.add(k);
    auto pid = [](const String& id) { return ParameterID { id, 1 }; };
    l.add(std::make_unique<AudioParameterChoice>(pid("genre"), "Genre", genres, 0));
    l.add(std::make_unique<AudioParameterChoice>(pid("key"), "Key", keys, 0));
    l.add(std::make_unique<AudioParameterChoice>(pid("tonality"), "Tonality", StringArray(kTonality, 2), 0));
    l.add(std::make_unique<AudioParameterChoice>(pid("mode"), "Mode", StringArray { "Loop", "Song" }, 0));
    l.add(std::make_unique<AudioParameterChoice>(pid("complexity"), "Complexity", StringArray { "Simple", "Complex" }, 0));
    l.add(std::make_unique<AudioParameterInt>(pid("loopBars"), "Loop Bars", 4, 32, 8));
    l.add(std::make_unique<AudioParameterFloat>(pid("minutes"), "Song Minutes", NormalisableRange<float>(1.0f, 5.0f, 0.1f), 2.5f));
    l.add(std::make_unique<AudioParameterFloat>(pid("density"), "Density", NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.5f));
    l.add(std::make_unique<AudioParameterFloat>(pid("swing"), "Swing", NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));
    l.add(std::make_unique<AudioParameterFloat>(pid("humanize"), "Humanize", NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));
    l.add(std::make_unique<AudioParameterInt>(pid("songSeed"), "Song Seed", 1, 999, 1));
    for (int p = 0; p < NumParts; ++p)
    {
        const String n = kPartNames[p];
        l.add(std::make_unique<AudioParameterChoice>(pid(partParam("type", p)), n + " Type", toArray(kPartTypes[(size_t)p]), 0));
        l.add(std::make_unique<AudioParameterInt>(pid(partParam("seed", p)), n + " Seed", 1, 999, 1));
        l.add(std::make_unique<AudioParameterBool>(pid(partParam("lock", p)), n + " Lock", false));
        l.add(std::make_unique<AudioParameterBool>(pid(partParam("mute", p)), n + " Mute", false));
    }
    l.add(std::make_unique<AudioParameterBool>(pid("preview"), "Preview Sound", true));
    l.add(std::make_unique<AudioParameterFloat>(pid("volume"), "Preview Volume", NormalisableRange<float>(-30.0f, 6.0f, 0.1f), -6.0f));
    l.add(std::make_unique<AudioParameterBool>(pid("midiOut"), "MIDI Out", true));
    return l;
}

GenreBenderProcessor::GenreBenderProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    apvts.addParameterListener("swing", this);
    for (int p = 0; p < NumParts; ++p) apvts.addParameterListener(partParam("mute", p), this);
    events.reserve(1 << 16);
    generateAll();                               // something to hear straight away
}

GenreBenderProcessor::~GenreBenderProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener("swing", this);
    for (int p = 0; p < NumParts; ++p) apvts.removeParameterListener(partParam("mute", p), this);
}

bool GenreBenderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

// ------------------------------------------------------------------------------------------------
gb::Settings GenreBenderProcessor::currentSettings() const
{
    auto ival = [this](const char* id) { return (int)apvts.getRawParameterValue(id)->load(); };
    auto fval = [this](const char* id) { return (double)apvts.getRawParameterValue(id)->load(); };
    Settings s;
    s.genre = ival("genre");
    s.key = ival("key");
    s.minor = ival("tonality") == 1;
    s.song = ival("mode") == 1;
    s.complex = ival("complexity") == 1;
    s.loopBars = ival("loopBars");
    s.minutes = fval("minutes");
    s.density = fval("density");
    s.humanize = fval("humanize");
    s.songSeed = ival("songSeed");
    s.bpm = bpm.load();
    for (int p = 0; p < NumParts; ++p)
    {
        s.partSeed[p] = (int)apvts.getRawParameterValue(partParam("seed", p))->load();
        s.partType[p] = (int)apvts.getRawParameterValue(partParam("type", p))->load();
    }
    return s;
}

void GenreBenderProcessor::regenerate(const gb::Settings& s, bool respectLocks, int onlyPart)
{
    plan = makePlan(s);
    genSettings = s;
    for (int p = 0; p < NumParts; ++p)
    {
        if (onlyPart >= 0 && p != onlyPart) continue;
        const bool locked = apvts.getRawParameterValue(partParam("lock", p))->load() > 0.5f;
        if (respectLocks && locked && !notes[p].empty()) continue;
        notes[p] = generatePart(s, plan, p);
    }
    rebuildPlayback();
    sendChangeMessage();
}

void GenreBenderProcessor::generateAll()
{
    regenerate(currentSettings(), true, -1);
}

void GenreBenderProcessor::rerollPart(int part)
{
    if (part < 0 || part >= NumParts) return;
    auto* seed = apvts.getParameter(partParam("seed", part));
    const int cur = (int)apvts.getRawParameterValue(partParam("seed", part))->load();
    const int next = cur % 999 + 1;
    seed->setValueNotifyingHost(seed->convertTo0to1((float)next));
    Settings s = genSettings;
    s.partSeed[part] = next;
    s.partType[part] = (int)apvts.getRawParameterValue(partParam("type", part))->load();
    s.humanize = (double)apvts.getRawParameterValue("humanize")->load();
    s.density = (double)apvts.getRawParameterValue("density")->load();
    regenerate(s, false, part);
}

void GenreBenderProcessor::newSongSeed()
{
    auto* p = apvts.getParameter("songSeed");
    const int cur = (int)apvts.getRawParameterValue("songSeed")->load();
    int next = juce::Random::getSystemRandom().nextInt(999) + 1;
    if (next == cur) next = cur % 999 + 1;
    p->setValueNotifyingHost(p->convertTo0to1((float)next));
    generateAll();
}

std::vector<gb::Note> GenreBenderProcessor::getPartNotes(int part) const
{
    if (part < 0 || part >= NumParts) return {};
    return notes[part];
}

void GenreBenderProcessor::setPartNotes(int part, std::vector<gb::Note> n)
{
    if (part < 0 || part >= NumParts) return;
    for (auto& x : n) x.part = part;
    std::sort(n.begin(), n.end(), [](const Note& a, const Note& b) { return a.start < b.start; });
    notes[part] = std::move(n);
    rebuildPlayback();
}

juce::String GenreBenderProcessor::describe() const
{
    const Settings& s = genSettings;
    return juce::String(kGenres[s.genre]) + "  |  " + kKeyNames[s.key] + (s.minor ? " minor" : " major") + "  |  "
         + (s.song ? "Song" : "Loop") + (s.complex ? " (complex)" : "") + "  |  " + juce::String(plan.totalBars()) + " bars  |  song seed "
         + juce::String(s.songSeed);
}

// ------------------------------------------------------------------------------------------------
void GenreBenderProcessor::parameterChanged(const juce::String&, float)
{
    triggerAsyncUpdate();                       // swing or a mute changed: rebuild on the message thread
}

void GenreBenderProcessor::handleAsyncUpdate()
{
    rebuildPlayback();
}

void GenreBenderProcessor::rebuildPlayback()
{
    const double swing = (double)apvts.getRawParameterValue("swing")->load();
    const double L = plan.totalBars() * 4.0;
    std::vector<Ev> ev;
    for (int p = 0; p < NumParts; ++p)
    {
        if (apvts.getRawParameterValue(partParam("mute", p))->load() > 0.5f) continue;
        for (auto& n : notes[p])
        {
            double on = playStart(n, swing);
            if (on >= L) continue;
            double off = std::min(on + n.length, L - 1e-6);
            if (off <= on) off = on + 1e-4;
            ev.push_back({ on, n.pitch, n.velocity, p, true });
            ev.push_back({ off, n.pitch, 0.0f, p, false });
        }
    }
    std::sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) {
        if (a.beat != b.beat) return a.beat < b.beat;
        return a.on < b.on;                      // note-offs first at the same moment
    });
    {
        const juce::SpinLock::ScopedLockType sl(playLock);
        events.swap(ev);
        loopBeats = L;
    }
    wantAllOff = true;
}

// ------------------------------------------------------------------------------------------------
void GenreBenderProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    synth.prepare(sampleRate);
    for (auto& a : active) for (auto& x : a) x = false;
    lastEnd = -1.0;
    wasPlaying = false;
}

void GenreBenderProcessor::setPreviewPlaying(bool on)
{
    if (on) internalBeat = 0.0;
    previewPlaying = on;
}

void GenreBenderProcessor::emit(const Ev& e, juce::MidiBuffer& midi, int offset, bool midiOut)
{
    const int ch = e.part + 1;
    if (e.on)
    {
        if (active[e.part][e.pitch])            // retrigger: close the old one first
        {
            if (midiOut) midi.addEvent(juce::MidiMessage::noteOff(ch, e.pitch), offset);
            synth.noteOff(e.part, e.pitch);
        }
        if (midiOut) midi.addEvent(juce::MidiMessage::noteOn(ch, e.pitch, (juce::uint8)juce::jlimit(1, 127, (int)std::lround(e.vel * 127.0f))), offset);
        synth.noteOn(e.part, e.pitch, e.vel);
        active[e.part][e.pitch] = true;
    }
    else if (active[e.part][e.pitch])
    {
        if (midiOut) midi.addEvent(juce::MidiMessage::noteOff(ch, e.pitch), offset);
        synth.noteOff(e.part, e.pitch);
        active[e.part][e.pitch] = false;
    }
}

void GenreBenderProcessor::sendAllOff(juce::MidiBuffer& midi, int offset)
{
    for (int p = 0; p < NumParts; ++p)
        for (int k = 0; k < 128; ++k)
            if (active[p][k])
            {
                midi.addEvent(juce::MidiMessage::noteOff(p + 1, k), offset);
                active[p][k] = false;
            }
    synth.allOff();
}

void GenreBenderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    midi.clear();                                // we are the source of notes

    bool hostPlaying = false;
    double ppq = 0.0, hostBpm = bpm.load();
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) if (*b > 20.0) hostBpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    bpm = hostBpm;
    const double spb = sr * 60.0 / hostBpm;
    const double blockBeats = n / spb;
    const bool midiOut = apvts.getRawParameterValue("midiOut")->load() > 0.5f;
    const bool preview = apvts.getRawParameterValue("preview")->load() > 0.5f;
    const float gain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("volume")->load()) * (preview ? 1.0f : 0.0f);

    bool playing = false;
    double b0 = 0.0;
    if (hostPlaying) { playing = true; b0 = ppq; }
    else if (previewPlaying.load()) { playing = true; b0 = internalBeat; internalBeat += blockBeats; }

    if (wantAllOff.exchange(false)) sendAllOff(midi, 0);

    struct Pending { int offset; Ev e; };
    Pending pend[1024];
    int np = 0;

    if (!playing)
    {
        if (wasPlaying) sendAllOff(midi, 0);
        wasPlaying = false;
        lastEnd = -1.0;
        playBeat = -1.0;
    }
    else
    {
        if (!wasPlaying || std::abs(b0 - lastEnd) > 1e-3) sendAllOff(midi, 0);   // started or jumped
        wasPlaying = true;
        const double b1 = b0 + blockBeats;
        lastEnd = b1;

        const juce::SpinLock::ScopedTryLockType tl(playLock);
        if (tl.isLocked() && loopBeats > 0.0 && !events.empty())
        {
            const double L = loopBeats;
            playBeat = std::fmod(std::max(0.0, b0), L);
            double start = std::max(0.0, b0);
            while (start < b1 && np < 1024)
            {
                const double cycle = std::floor(start / L) * L;
                const double p0 = start - cycle;
                const double segEnd = std::min(b1, cycle + L);
                const double p1 = segEnd - cycle;
                if (p0 < 1e-9 && start > b0 + 1e-9)    // wrapped round: close anything still ringing
                {
                    const int off = juce::jlimit(0, n - 1, (int)((start - b0) * spb));
                    for (int pp = 0; pp < NumParts; ++pp)
                        for (int k = 0; k < 128 && np < 1024; ++k)
                            if (active[pp][k]) pend[np++] = { off, { 0.0, k, 0.0f, pp, false } };
                }
                auto it = std::lower_bound(events.begin(), events.end(), p0, [](const Ev& e, double b) { return e.beat < b; });
                for (; it != events.end() && it->beat < p1 && np < 1024; ++it)
                {
                    const int off = juce::jlimit(0, n - 1, (int)((cycle + it->beat - b0) * spb));
                    pend[np++] = { off, *it };
                }
                start = segEnd;
            }
        }
    }

    // render the preview synth in slices between events (sample accurate)
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    float scratchR[4096];
    int pos = 0, k = 0;
    while (pos < n)
    {
        while (k < np && pend[k].offset <= pos) { emit(pend[k].e, midi, pend[k].offset, midiOut); ++k; }
        const int nextOff = k < np ? pend[k].offset : n;
        const int len = std::max(1, std::min(nextOff, n) - pos);
        if (R != nullptr) synth.render(L + pos, R + pos, len, gain);
        else
        {
            const int m = std::min(len, 4096);
            std::fill(scratchR, scratchR + m, 0.0f);
            synth.render(L + pos, scratchR, m, gain);
        }
        pos += len;
    }
    while (k < np) { emit(pend[k].e, midi, pend[k].offset, midiOut); ++k; }

    // gentle safety on the preview sound
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = std::tanh(d[i]);
    }
    if (!midiOut) midi.clear();
}

// ------------------------------------------------------------------------------------------------
bool GenreBenderProcessor::writeMidiFile(const juce::File& file, int partOrAll) const
{
    const double swing = (double)apvts.getRawParameterValue("swing")->load();
    const int ppqTicks = 960;
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(ppqTicks);
    juce::MidiMessageSequence tempo;
    tempo.addEvent(juce::MidiMessage::tempoMetaEvent((int)std::lround(60000000.0 / bpm.load())), 0);
    tempo.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0);
    tempo.addEvent(juce::MidiMessage::textMetaEvent(3, "Genre Bender - " + describe()), 0);
    mf.addTrack(tempo);
    int written = 0;
    for (int p = 0; p < NumParts; ++p)
    {
        if (partOrAll >= 0 && p != partOrAll) continue;
        if (notes[p].empty()) continue;
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, kPartNames[p]), 0);
        for (auto& nt : notes[p])
        {
            const double on = playStart(nt, swing) * ppqTicks;
            const double off = on + nt.length * ppqTicks;
            const int vel = juce::jlimit(1, 127, (int)std::lround(nt.velocity * 127.0f));
            seq.addEvent(juce::MidiMessage::noteOn(p + 1, nt.pitch, (juce::uint8)vel), on);
            seq.addEvent(juce::MidiMessage::noteOff(p + 1, nt.pitch), off);
        }
        seq.updateMatchedPairs();
        mf.addTrack(seq);
        ++written;
    }
    if (written == 0) return false;
    file.deleteFile();
    juce::FileOutputStream out(file);
    if (!out.openedOk()) return false;
    return mf.writeTo(out, 1);
}

juce::File GenreBenderProcessor::tempMidiFile(int partOrAll) const
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Genre Bender");
    dir.createDirectory();
    const juce::String what = partOrAll < 0 ? juce::String("Full Band") : juce::String(kPartNames[partOrAll]);
    const juce::String name = what + " - " + kGenres[genSettings.genre] + " " + kKeyNames[genSettings.key]
                            + (genSettings.minor ? "m" : "") + " s" + juce::String(genSettings.songSeed);
    return dir.getChildFile(juce::File::createLegalFileName(name) + ".mid");
}

// ------------------------------------------------------------------------------------------------
void GenreBenderProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement root("GenreBender");
    root.setAttribute("version", 1);
    if (auto xml = apvts.copyState().createXml()) root.addChildElement(xml.release());
    auto* gen = root.createNewChildElement("Gen");
    const Settings& s = genSettings;
    gen->setAttribute("key", s.key); gen->setAttribute("minor", s.minor); gen->setAttribute("genre", s.genre);
    gen->setAttribute("song", s.song); gen->setAttribute("complex", s.complex); gen->setAttribute("loopBars", s.loopBars);
    gen->setAttribute("minutes", s.minutes); gen->setAttribute("bpm", s.bpm); gen->setAttribute("density", s.density);
    gen->setAttribute("humanize", s.humanize); gen->setAttribute("songSeed", s.songSeed);
    for (int p = 0; p < NumParts; ++p)
    {
        auto* e = root.createNewChildElement("Part");
        e->setAttribute("i", p);
        e->setAttribute("seed", s.partSeed[p]);
        e->setAttribute("type", s.partType[p]);
        juce::String txt;
        txt.preallocateBytes(notes[p].size() * 24);
        for (auto& n : notes[p])
            txt << n.pitch << ',' << juce::String(n.start, 4) << ',' << juce::String(n.length, 4) << ','
                << juce::String(n.velocity, 3) << ',' << juce::String(n.nudge, 4) << ';';
        e->addTextElement(txt);
    }
    copyXmlToBinary(root, destData);
}

void GenreBenderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto root = getXmlFromBinary(data, sizeInBytes);
    if (root == nullptr || !root->hasTagName("GenreBender")) return;
    if (auto* st = root->getChildByName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*st));
    Settings s;
    if (auto* gen = root->getChildByName("Gen"))
    {
        s.key = gen->getIntAttribute("key"); s.minor = gen->getBoolAttribute("minor"); s.genre = gen->getIntAttribute("genre");
        s.song = gen->getBoolAttribute("song"); s.complex = gen->getBoolAttribute("complex"); s.loopBars = gen->getIntAttribute("loopBars", 8);
        s.minutes = gen->getDoubleAttribute("minutes", 2.5); s.bpm = gen->getDoubleAttribute("bpm", 90.0);
        s.density = gen->getDoubleAttribute("density", 0.5); s.humanize = gen->getDoubleAttribute("humanize", 0.4);
        s.songSeed = gen->getIntAttribute("songSeed", 1);
    }
    for (auto* e : root->getChildWithTagNameIterator("Part"))
    {
        const int p = e->getIntAttribute("i", -1);
        if (p < 0 || p >= NumParts) continue;
        s.partSeed[p] = e->getIntAttribute("seed", 1);
        s.partType[p] = e->getIntAttribute("type", 0);
        std::vector<Note> v;
        juce::StringArray items;
        items.addTokens(e->getAllSubText(), ";", "");
        for (auto& it : items)
        {
            juce::StringArray f;
            f.addTokens(it, ",", "");
            if (f.size() < 5) continue;
            Note n;
            n.pitch = juce::jlimit(0, 127, f[0].getIntValue());
            n.start = f[1].getDoubleValue(); n.length = std::max(0.01, f[2].getDoubleValue());
            n.velocity = juce::jlimit(0.01f, 1.0f, f[3].getFloatValue()); n.nudge = f[4].getFloatValue();
            n.part = p;
            v.push_back(n);
        }
        notes[p] = std::move(v);
    }
    genSettings = s;
    plan = makePlan(s);
    rebuildPlayback();
    sendChangeMessage();
}

juce::AudioProcessorEditor* GenreBenderProcessor::createEditor()
{
    return new GenreBenderEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GenreBenderProcessor();
}
