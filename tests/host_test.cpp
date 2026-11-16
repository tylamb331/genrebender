// Test app: drives the processor directly (playback, MIDI out, edits, state, MIDI files, editor
// snapshot), then loads the built VST3 the way a host would.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_processors/juce_audio_processors.h>

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 100; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm(bpm); p.setIsPlaying(playing); p.setPpqPosition(ppq);
        return p;
    }
};

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

struct RunResult { int ons[7] = {}; int offs[7] = {}; int hanging = 0; float peak = 0; bool finite = true; };

static RunResult run(juce::AudioProcessor& p, Head& head, double seconds, int block, double startPpq = 0)
{
    RunResult r;
    const double sr = 48000;
    p.setPlayHead(&head);
    p.setPlayConfigDetails(0, 2, sr, block);
    p.prepareToPlay(sr, block);
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer midi;
    bool on[17][128] = {};
    head.ppq = startPpq;
    const int blocks = (int)(seconds * sr / block);
    for (int b = 0; b < blocks; ++b)
    {
        midi.clear();
        p.processBlock(buf, midi);
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            const int ch = msg.getChannel();
            if (ch < 1 || ch > 6) { r.finite = false; continue; }
            if (msg.isNoteOn()) { r.ons[ch]++; on[ch][msg.getNoteNumber()] = true; }
            else if (msg.isNoteOff()) { r.offs[ch]++; on[ch][msg.getNoteNumber()] = false; }
        }
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < block; ++i)
            {
                const float v = buf.getSample(c, i);
                if (!std::isfinite(v)) r.finite = false;
                r.peak = std::max(r.peak, std::abs(v));
            }
        head.ppq += block / sr * head.bpm / 60.0;
    }
    // stop: everything must be released
    head.playing = false;
    midi.clear();
    p.processBlock(buf, midi);
    for (const auto m : midi) { const auto msg = m.getMessage(); if (msg.isNoteOff()) on[msg.getChannel()][msg.getNoteNumber()] = false; }
    for (int c = 1; c <= 6; ++c) for (int k = 0; k < 128; ++k) if (on[c][k]) r.hanging++;
    head.playing = true;
    return r;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String outDir = argc > 1 ? argv[1] : "/tmp";

    // ---------- the processor itself ----------
    {
        GenreBenderProcessor p;
        auto set = [&](const char* id, float real) { auto* prm = p.apvts.getParameter(id); prm->setValueNotifyingHost(prm->convertTo0to1(real)); };
        set("genre", 1); set("tonality", 1); set("mode", 1); set("complexity", 1); set("minutes", 2.0f); set("songSeed", 7);
        p.generateAll();
        const double beats = p.getTotalBeats();
        printf("song: %s (%.0f beats)\n", p.describe().toRawUTF8(), beats);
        Head head; head.bpm = 100;
        auto r = run(p, head, 20.0, 512);
        int total = 0; for (int c = 1; c <= 6; ++c) total += r.ons[c];
        printf("20 s at 100 BPM: note-ons per part: keys %d bass %d lead %d guitar %d pad %d arp %d, peak %.2f, hanging %d\n",
               r.ons[1], r.ons[2], r.ons[3], r.ons[4], r.ons[5], r.ons[6], r.peak, r.hanging);
        CHECK(total > 100, "too few notes played (%d)", total);
        CHECK(r.finite, "non-finite audio or bad channel");
        CHECK(r.hanging == 0, "%d notes left hanging after stop", r.hanging);
        CHECK(r.peak > 0.01f && r.peak <= 1.0f, "preview level %.3f", r.peak);

        // odd block sizes, and a loop wrap (start just before the end of the song)
        Head h2; h2.bpm = 140;
        auto r2 = run(p, h2, 6.0, 37, beats - 3.0);
        CHECK(r2.hanging == 0 && r2.finite, "wrap / odd block: hanging %d", r2.hanging);

        // MIDI out off -> no MIDI, preview off -> silence
        set("midiOut", 0); set("preview", 0);
        Head h3; auto r3 = run(p, h3, 3.0, 256);
        int t3 = 0; for (int c = 1; c <= 6; ++c) t3 += r3.ons[c];
        CHECK(t3 == 0 && r3.peak < 1e-6f, "midi out / preview switches (notes %d, peak %g)", t3, r3.peak);
        set("midiOut", 1); set("preview", 1);

        // mute a part
        set("mute1", 1);
        Head h4; auto r4 = run(p, h4, 8.0, 512);
        CHECK(r4.ons[2] == 0 && r4.ons[1] > 0, "mute bass: bass %d keys %d", r4.ons[2], r4.ons[1]);
        set("mute1", 0);

        // lock keys, re-roll bass: keys must not change; bass must
        set("lock0", 1);
        auto keysBefore = p.getPartNotes(gb::Keys), bassBefore = p.getPartNotes(gb::Bass);
        p.rerollPart(gb::Bass);
        auto keysAfter = p.getPartNotes(gb::Keys), bassAfter = p.getPartNotes(gb::Bass);
        bool keysSame = keysBefore.size() == keysAfter.size();
        for (size_t i = 0; keysSame && i < keysBefore.size(); ++i) keysSame = keysBefore[i].pitch == keysAfter[i].pitch && keysBefore[i].start == keysAfter[i].start;
        bool bassDiff = bassBefore.size() != bassAfter.size();
        for (size_t i = 0; !bassDiff && i < bassBefore.size(); ++i) bassDiff = bassBefore[i].pitch != bassAfter[i].pitch;
        CHECK(keysSame && bassDiff, "re-roll: keys same %d, bass changed %d", keysSame, bassDiff);
        set("songSeed", 99);
        p.generateAll();                                  // keys locked: stays
        auto keysLocked = p.getPartNotes(gb::Keys);
        CHECK(keysLocked.size() == keysBefore.size(), "lock ignored by Generate");
        set("lock0", 0);

        // edit: add a note, delete one, then save/restore state
        auto lead = p.getPartNotes(gb::Lead);
        gb::Note extra; extra.pitch = 99; extra.start = 1.0; extra.length = 0.5; extra.velocity = 0.9f;
        lead.push_back(extra);
        lead.erase(lead.begin());
        p.setPartNotes(gb::Lead, lead);
        juce::MemoryBlock state;
        p.getStateInformation(state);
        GenreBenderProcessor q;
        q.setStateInformation(state.getData(), (int)state.getSize());
        bool same = true;
        for (int part = 0; part < gb::NumParts; ++part)
        {
            auto a = p.getPartNotes(part), b = q.getPartNotes(part);
            if (a.size() != b.size()) { same = false; break; }
            for (size_t i = 0; i < a.size(); ++i)
                if (a[i].pitch != b[i].pitch || std::abs(a[i].start - b[i].start) > 1e-3 || std::abs(a[i].length - b[i].length) > 1e-3) { same = false; break; }
        }
        CHECK(same, "state round trip lost notes");
        CHECK(q.describe() == p.describe(), "state round trip: %s vs %s", q.describe().toRawUTF8(), p.describe().toRawUTF8());
        bool has99 = false; for (auto& n : q.getPartNotes(gb::Lead)) if (n.pitch == 99) has99 = true;
        CHECK(has99, "edited note missing after reload");
        printf("state: %zu bytes, edits survive reload\n", state.getSize());

        // MIDI files
        auto f = juce::File(outDir).getChildFile("full band.mid");
        CHECK(p.writeMidiFile(f, -1), "write full band");
        juce::MidiFile mf;
        {
            juce::FileInputStream in(f);
            CHECK(in.openedOk() && mf.readFrom(in), "read back midi");
        }
        int fileNotes = 0;
        for (int t = 0; t < mf.getNumTracks(); ++t)
            for (auto* e : *mf.getTrack(t)) if (e->message.isNoteOn()) ++fileNotes;
        int procNotes = 0; for (int part = 0; part < gb::NumParts; ++part) procNotes += (int)p.getPartNotes(part).size();
        printf("midi file: %d tracks, %d notes (song has %d)\n", mf.getNumTracks(), fileNotes, procNotes);
        CHECK(fileNotes == procNotes && mf.getNumTracks() == 7, "midi file content");
        auto f2 = juce::File(outDir).getChildFile("bass only.mid");
        CHECK(p.writeMidiFile(f2, gb::Bass), "write bass");

        // editor snapshot (ALL view and LEAD view)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
            ed->setSize(1180, 760);
            auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            juce::FileOutputStream o(juce::File(outDir).getChildFile("editor_all.png"));
            o.setPosition(0); o.truncate();
            png.writeImageToStream(img, o);
            for (auto* c : ed->getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*>(c))
                    if (b->getButtonText() == "LEAD" && b->onClick) b->onClick();
            auto img2 = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
            juce::FileOutputStream o2(juce::File(outDir).getChildFile("editor_lead.png"));
            o2.setPosition(0); o2.truncate();
            png.writeImageToStream(img2, o2);
        }
    }

    // ---------- the built VST3, loaded like a host would ----------
    if (argc > 2)
    {
        juce::VST3PluginFormat fmt;
        juce::OwnedArray<juce::PluginDescription> found;
        fmt.findAllTypesForFile(found, argv[2]);
        CHECK(found.size() == 1, "VST3 scan found %d plugins", found.size());
        if (found.size() == 1)
        {
            auto* d = found[0];
            printf("VST3: '%s' by '%s', instrument=%d, ins=%d outs=%d\n", d->name.toRawUTF8(), d->manufacturerName.toRawUTF8(), d->isInstrument, d->numInputChannels, d->numOutputChannels);
            juce::String err;
            auto inst = fmt.createInstanceFromDescription(*d, 48000, 512, err);
            CHECK(inst != nullptr, "VST3 instantiate: %s", err.toRawUTF8());
            if (inst)
            {
                Head h; h.bpm = 92;
                auto r = run(*inst, h, 10.0, 512);
                int t = 0; for (int c = 1; c <= 6; ++c) t += r.ons[c];
                printf("VST3 playback: %d note-ons, peak %.2f, hanging %d, params %d\n", t, r.peak, r.hanging, (int)inst->getParameters().size());
                CHECK(t > 20 && r.finite && r.hanging == 0, "VST3 playback");
                juce::MemoryBlock st; inst->getStateInformation(st);
                inst->setStateInformation(st.getData(), (int)st.getSize());
                CHECK(inst->hasEditor(), "no editor");
                std::unique_ptr<juce::AudioProcessorEditor> ed(inst->createEditorIfNeeded());
                CHECK(ed != nullptr, "VST3 editor");
                if (ed) printf("VST3 editor %dx%d\n", ed->getWidth(), ed->getHeight());
            }
        }
    }
    printf(fails ? "TESTS FAILED (%d)\n" : "ALL TESTS PASSED\n", fails);
    return fails ? 1 : 0;
}
