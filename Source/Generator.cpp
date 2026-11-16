// Genre Bender - generator engine. See Generator.h.
#include "Generator.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace gb
{

const char* const kPartNames[NumParts] = { "Keys", "Bass", "Lead", "Guitar", "Pad", "Arp" };
const char* const kGenres[] = { "Vaporwave", "City Pop", "Future Funk", "90s R&B", "Boom Bap Soul",
                                "Smooth Jazz", "Lounge / Elevator", "80s R&B", "Motown" };
const int kNumGenres = 9;
const char* const kKeyNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

const std::vector<std::vector<std::string>> kPartTypes = {
    { "Electric Piano", "Piano", "Organ", "Clav (stabs)" },
    { "Genre Default", "Electric (fingers)", "Synth (boogie)", "Upright (walking)", "808 (long)" },
    { "Genre Default", "Saxophone", "Synth Lead", "Bells / Vibes", "Vocal Line" },
    { "Funk Chops (16ths)", "Clean Stabs", "Picked Arpeggio" },
    { "Warm Pad", "Strings" },
    { "Up", "Up & Down", "Skip" },
};

namespace
{
struct Q { const char* name; std::vector<int> iv; };
const std::vector<Q> kQualities = {
    { "maj", { 0, 4, 7 } }, { "m", { 0, 3, 7 } }, { "6", { 0, 4, 7, 9 } },
    { "maj7", { 0, 4, 7, 11 } }, { "maj9", { 0, 4, 7, 11, 14 } }, { "6/9", { 0, 4, 7, 9, 14 } },
    { "m7", { 0, 3, 7, 10 } }, { "m9", { 0, 3, 7, 10, 14 } }, { "m11", { 0, 3, 7, 10, 14, 17 } },
    { "7", { 0, 4, 7, 10 } }, { "9", { 0, 4, 7, 10, 14 } }, { "13", { 0, 4, 10, 14, 21 } },
    { "7sus4", { 0, 5, 7, 10 } }, { "m7b5", { 0, 3, 6, 10 } },
};

const std::vector<int>& intervals(const char* q)
{
    for (auto& x : kQualities) if (std::strcmp(x.name, q) == 0) return x.iv;
    return kQualities[3].iv;
}

using Prog = std::vector<Chord>;
struct GenreProgs { std::vector<Prog> major, minor; };

const std::vector<GenreProgs> kProgs = {
    { // Vaporwave
      { { { 0, "maj7" }, { 9, "m7" }, { 2, "m9" }, { 7, "9" } },
        { { 5, "maj7" }, { 4, "m7" }, { 2, "m7" }, { 0, "maj9" } },
        { { 5, "maj9" }, { 7, "7sus4" }, { 4, "m7" }, { 9, "m9" } } },
      { { { 0, "m9" }, { 8, "maj7" }, { 5, "m9" }, { 7, "7sus4" } },
        { { 0, "m7" }, { 10, "9" }, { 8, "maj7" }, { 7, "m7" } },
        { { 8, "maj9" }, { 10, "6" }, { 0, "m9" }, { 0, "m9" } } } },
    { // City Pop
      { { { 5, "maj7" }, { 4, "7" }, { 9, "m7" }, { 7, "9" } },
        { { 2, "m7" }, { 7, "9" }, { 0, "maj7" }, { 9, "7" } },
        { { 5, "maj9" }, { 7, "9" }, { 4, "m7" }, { 9, "m9" } } },
      { { { 8, "maj7" }, { 7, "7" }, { 0, "m7" }, { 0, "m7" } },
        { { 8, "maj7" }, { 7, "7" }, { 0, "m7" }, { 3, "7" } },
        { { 5, "m9" }, { 10, "9" }, { 3, "maj7" }, { 8, "maj7" } } } },
    { // Future Funk
      { { { 0, "maj7" }, { 2, "m7" }, { 4, "m7" }, { 5, "maj7" } },
        { { 5, "maj9" }, { 4, "m7" }, { 2, "m9" }, { 7, "9" } },
        { { 0, "maj9" }, { 9, "m9" }, { 5, "maj9" }, { 7, "7sus4" } } },
      { { { 0, "m9" }, { 5, "9" }, { 0, "m9" }, { 5, "9" } },
        { { 0, "m7" }, { 3, "maj7" }, { 5, "m7" }, { 7, "m7" } },
        { { 8, "maj7" }, { 10, "6" }, { 0, "m9" }, { 7, "7sus4" } } } },
    { // 90s R&B
      { { { 5, "maj9" }, { 4, "m7" }, { 2, "m9" }, { 0, "maj9" } },
        { { 2, "m9" }, { 7, "13" }, { 0, "maj9" }, { 9, "7" } },
        { { 0, "maj9" }, { 5, "maj9" }, { 2, "m11" }, { 7, "7sus4" } } },
      { { { 0, "m9" }, { 5, "m11" }, { 10, "9" }, { 3, "maj9" } },
        { { 0, "m11" }, { 8, "maj9" }, { 10, "9" }, { 7, "7sus4" } },
        { { 5, "m9" }, { 10, "13" }, { 3, "maj9" }, { 0, "m9" } } } },
    { // Boom Bap Soul
      { { { 0, "maj7" }, { 9, "m7" }, { 5, "maj7" }, { 7, "7sus4" } },
        { { 2, "m7" }, { 5, "maj7" }, { 0, "maj7" }, { 0, "maj7" } },
        { { 5, "maj7" }, { 4, "m7" }, { 9, "m7" }, { 2, "m7" } } },
      { { { 0, "m7" }, { 5, "m7" }, { 0, "m7" }, { 7, "7" } },
        { { 0, "m9" }, { 10, "maj7" }, { 8, "maj7" }, { 7, "7sus4" } },
        { { 0, "m7" }, { 8, "maj7" }, { 5, "m7" }, { 7, "m7" } } } },
    { // Smooth Jazz
      { { { 2, "m9" }, { 7, "9" }, { 0, "maj9" }, { 9, "7" } },
        { { 5, "maj9" }, { 4, "m7" }, { 9, "m9" }, { 2, "m9" } },
        { { 0, "6/9" }, { 4, "m7b5" }, { 9, "7" }, { 2, "m9" } } },
      { { { 0, "m9" }, { 5, "9" }, { 10, "maj7" }, { 3, "maj7" } },
        { { 2, "m7b5" }, { 7, "7" }, { 0, "m9" }, { 0, "m9" } },
        { { 5, "m9" }, { 10, "9" }, { 3, "maj9" }, { 8, "maj7" } } } },
    { // Lounge / Elevator
      { { { 0, "6/9" }, { 9, "m9" }, { 2, "m9" }, { 7, "9" } },
        { { 0, "maj7" }, { 4, "m7" }, { 5, "maj7" }, { 7, "7sus4" } },
        { { 5, "6/9" }, { 7, "9" }, { 0, "maj9" }, { 9, "m7" } } },
      { { { 0, "m9" }, { 2, "m7b5" }, { 7, "7" }, { 0, "m9" } },
        { { 0, "m7" }, { 5, "m7" }, { 10, "9" }, { 3, "maj7" } },
        { { 8, "maj7" }, { 7, "7" }, { 0, "m9" }, { 0, "m9" } } } },
    { // 80s R&B
      { { { 0, "maj9" }, { 2, "m9" }, { 4, "m7" }, { 5, "maj9" } },
        { { 5, "maj7" }, { 7, "9" }, { 4, "m7" }, { 9, "m7" } },
        { { 2, "m9" }, { 7, "9" }, { 0, "maj9" }, { 0, "maj9" } } },
      { { { 0, "m9" }, { 5, "9" }, { 8, "maj7" }, { 7, "7sus4" } },
        { { 0, "m7" }, { 10, "9" }, { 3, "maj7" }, { 5, "m9" } },
        { { 8, "maj9" }, { 10, "9" }, { 0, "m9" }, { 0, "m9" } } } },
    { // Motown
      { { { 0, "maj" }, { 9, "m7" }, { 5, "maj" }, { 7, "7" } },
        { { 0, "maj" }, { 5, "maj" }, { 7, "7" }, { 5, "maj" } },
        { { 0, "6" }, { 4, "m7" }, { 5, "maj7" }, { 7, "7" } } },
      { { { 0, "m7" }, { 5, "7" }, { 0, "m7" }, { 7, "7" } },
        { { 0, "m" }, { 10, "maj" }, { 8, "maj" }, { 7, "7" } },
        { { 5, "m7" }, { 10, "7" }, { 3, "maj7" }, { 7, "7" } } } },
};

struct Feel { const char* bass; const char* lead; const char* keys; bool dorian; };
const Feel kFeel[] = {
    { "smooth", "synth", "held", false }, { "slap", "synth", "sync", false }, { "octave", "synth", "chop", true },
    { "sparse", "vocal", "lush", true },  { "boom", "sax", "hits", true },    { "smooth", "sax", "comp", false },
    { "walk", "bells", "comp", false },   { "slap", "sax", "sync", false },   { "motown", "vocal", "hits", false },
};

struct Hit { double s, l; };
struct BassHit { double s, l; int d; };
using Cell = std::vector<Hit>;
using BassCell = std::vector<BassHit>;

const std::map<std::string, std::vector<Cell>> kKeysRhy = {
    { "held", { { { 0, 4 } }, { { 0, 2 }, { 2, 2 } }, { { 0, 3.5 }, { 3.5, 0.5 } } } },
    { "sync", { { { 0, 1.5 }, { 1.5, 1 }, { 2.5, 1.5 } }, { { 0, 0.75 }, { 0.75, 1.25 }, { 2.5, 1.5 } }, { { 0, 1.5 }, { 2.5, 0.5 }, { 3, 1 } } } },
    { "chop", { { { 0, 0.4 }, { 0.75, 0.4 }, { 1.5, 0.4 }, { 2.5, 0.4 }, { 3, 0.4 }, { 3.5, 0.4 } },
                { { 0, 0.4 }, { 0.5, 0.4 }, { 1.5, 0.4 }, { 2, 0.4 }, { 3, 0.4 }, { 3.75, 0.25 } },
                { { 0.5, 0.4 }, { 1, 0.4 }, { 1.75, 0.4 }, { 2.5, 0.4 }, { 3.5, 0.4 } } } },
    { "lush", { { { 0, 4 } }, { { 0, 2.5 }, { 2.5, 1.5 } }, { { 0, 1.75 }, { 1.75, 2.25 } } } },
    { "hits", { { { 0, 1 }, { 2.5, 1 } }, { { 0, 1.5 }, { 3, 1 } }, { { 0, 0.75 }, { 1.5, 0.5 }, { 2.5, 1.5 } } } },
    { "comp", { { { 0, 1.5 }, { 2.5, 1.5 } }, { { 0.5, 1 }, { 2, 0.5 }, { 3, 1 } }, { { 0, 2.5 }, { 3.5, 0.5 } } } },
};

const std::map<std::string, std::vector<BassCell>> kBassRhy = {
    { "smooth", { { { 0, 1.5, 0 }, { 1.5, 0.5, 0 }, { 2, 1, 1 }, { 3, 1, 2 } }, { { 0, 2.5, 0 }, { 2.5, 0.5, 1 }, { 3, 1, 3 } } } },
    { "slap", { { { 0, 0.5, 0 }, { 0.5, 0.5, 2 }, { 1, 0.5, 0 }, { 1.75, 0.25, 2 }, { 2, 0.5, 0 }, { 2.5, 0.5, 2 }, { 3, 0.5, 1 }, { 3.5, 0.5, 3 } },
                { { 0, 0.75, 0 }, { 0.75, 0.25, 2 }, { 1.5, 0.5, 0 }, { 2, 0.5, 2 }, { 2.75, 0.75, 1 }, { 3.5, 0.5, 3 } } } },
    { "octave", { { { 0, 0.4, 0 }, { 0.5, 0.4, 2 }, { 1, 0.4, 0 }, { 1.5, 0.4, 2 }, { 2, 0.4, 0 }, { 2.5, 0.4, 2 }, { 3, 0.4, 0 }, { 3.5, 0.4, 2 } },
                  { { 0, 0.4, 0 }, { 0.5, 0.4, 2 }, { 1, 0.4, 0 }, { 1.5, 0.4, 2 }, { 2, 0.4, 0 }, { 2.5, 0.4, 2 }, { 3, 0.4, 1 }, { 3.5, 0.4, 3 } } } },
    { "sparse", { { { 0, 1.75, 0 }, { 2.5, 1, 0 }, { 3.5, 0.5, 3 } }, { { 0, 2.75, 0 }, { 3, 0.5, 1 }, { 3.5, 0.5, 3 } } } },
    { "boom", { { { 0, 1, 0 }, { 1.5, 0.5, 0 }, { 2.75, 1.25, 0 } }, { { 0, 1.5, 0 }, { 2.5, 0.5, 1 }, { 3, 1, 0 } } } },
    { "walk", { { { 0, 1, 0 }, { 1, 1, 1 }, { 2, 1, 2 }, { 3, 1, 3 } }, { { 0, 1, 0 }, { 1, 1, 2 }, { 2, 1, 1 }, { 3, 1, 3 } } } },
    { "motown", { { { 0, 0.75, 0 }, { 0.75, 0.25, 0 }, { 1, 0.5, 1 }, { 1.5, 0.5, 2 }, { 2, 0.75, 0 }, { 2.75, 0.25, 1 }, { 3, 0.5, 2 }, { 3.5, 0.5, 3 } },
                  { { 0, 1, 0 }, { 1, 0.5, 2 }, { 1.5, 0.5, 1 }, { 2, 1, 0 }, { 3, 0.5, 1 }, { 3.5, 0.5, 3 } } } },
    { "808", { { { 0, 3.5, 0 }, { 3.5, 0.5, 2 } }, { { 0, 1.5, 0 }, { 2.5, 1.5, 0 } } } },
};

const std::vector<Cell> kMelSimple = {
    { { 0, 1 }, { 1, 0.5 }, { 1.5, 1.5 }, { 4, 1 }, { 5, 1 }, { 6, 2 } },
    { { 0.5, 0.5 }, { 1, 1 }, { 2, 2 }, { 4.5, 0.5 }, { 5, 1 }, { 6, 2 } },
    { { 0, 1.5 }, { 1.5, 0.5 }, { 2, 1 }, { 4, 0.5 }, { 4.5, 0.5 }, { 5, 3 } },
};
const std::vector<Cell> kMelComplex = {
    { { 0, 0.5 }, { 0.5, 0.25 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 1 }, { 3.5, 0.5 }, { 4, 0.75 }, { 4.75, 0.25 }, { 5, 1 }, { 6, 1.5 } },
    { { 0.25, 0.25 }, { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 1 }, { 2.5, 0.5 }, { 3, 1 }, { 4.5, 0.5 }, { 5, 0.5 }, { 5.5, 0.5 }, { 6, 2 } },
    { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 0.5 }, { 2.5, 1.5 }, { 4, 0.5 }, { 4.5, 0.25 }, { 4.75, 0.75 }, { 5.5, 0.5 }, { 6, 1 }, { 7, 0.5 } },
};

int mod12(int x) { return ((x % 12) + 12) % 12; }

bool startsWith(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }
bool contains(const std::string& s, const char* p) { return s.find(p) != std::string::npos; }

std::vector<int> voice(int root, const char* q, const std::vector<int>& prev, double center, int top)
{
    const auto& iv = intervals(q);
    std::vector<int> base;
    for (int i : iv) base.push_back(root + 48 + i);
    std::vector<std::vector<int>> opts;
    std::vector<int> notes = base;
    for (size_t k = 0; k < base.size(); ++k)
    {
        opts.push_back(notes);
        std::rotate(notes.begin(), notes.begin() + 1, notes.end());
        notes.back() += 12;
    }
    auto avg = [](const std::vector<int>& c) { double a = 0; for (int x : c) a += x; return a / (double)c.size(); };
    double target = prev.empty() ? center : avg(prev);
    target = 0.7 * target + 0.3 * center;
    std::vector<int> best;
    double bestD = 1e9;
    for (auto o : opts)
    {
        while (avg(o) > target + 6) for (int& x : o) x -= 12;
        while (avg(o) < target - 6) for (int& x : o) x += 12;
        while (*std::max_element(o.begin(), o.end()) > top) for (int& x : o) x -= 12;
        const double d = std::fabs(avg(o) - target);
        if (d < bestD) { bestD = d; best = o; }
    }
    return best;
}

bool plays(int part, const Section& sec, bool complex)
{
    const std::string& n = sec.name;
    const double e = sec.energy;
    if (startsWith(n, "Loop")) return true;
    switch (part)
    {
        case Keys: return true;
        case Bass: return !startsWith(n, "Intro") && n != "Breakdown";
        case Lead: return !(n == "Intro" || n == "Pre-Chorus") || (n == "Intro" && complex);
        case Guitar: return e >= 0.6;
        case Pad: return n == "Intro" || n == "Outro" || n == "Breakdown" || n == "Bridge" || contains(n, "Chorus");
        case Arp: return e >= 0.9 || n == "Bridge";
    }
    return true;
}

struct Out
{
    std::vector<Note>& v;
    Rng& hr;
    double hum;
    int part;
    void add(int pitch, double start, double length, double vel)
    {
        while (pitch > 108) pitch -= 12;
        while (pitch < 21) pitch += 12;
        Note n;
        n.pitch = pitch;
        n.start = std::max(0.0, start);
        n.length = std::max(0.05, length * 0.96);
        n.velocity = (float)std::min(1.0, std::max(0.08, vel + hr.uniform(-0.1, 0.1) * hum));
        n.nudge = (float)(hr.uniform(-0.015, 0.045) * hum);
        n.part = part;
        v.push_back(n);
    }
};

std::vector<int> scaleSet(int key, bool minor, bool dorian)
{
    static const int maj[7] = { 0, 2, 4, 5, 7, 9, 11 }, dor[7] = { 0, 2, 3, 5, 7, 9, 10 }, aeo[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const int* st = !minor ? maj : (dorian ? dor : aeo);
    std::vector<int> r;
    for (int i = 0; i < 7; ++i) r.push_back(mod12(key + st[i]));
    return r;
}

bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

} // namespace

std::vector<int> chordTones(int root, const char* quality)
{
    std::vector<int> r;
    for (int i : intervals(quality)) r.push_back(mod12(root + i));
    return r;
}

double playStart(const Note& n, double swing)
{
    double t = n.start;
    const double f = t - std::floor(t);
    if (std::fabs(f - 0.5) < 1e-6) t += swing * 0.16;
    else if (std::fabs(f - 0.25) < 1e-6 || std::fabs(f - 0.75) < 1e-6) t += swing * 0.08;
    return std::max(0.0, t + n.nudge);
}

// ------------------------------------------------------------------------------------------------
Plan makePlan(const Settings& s)
{
    const int g = std::max(0, std::min(kNumGenres - 1, s.genre));
    Rng rng((uint64_t)s.songSeed * 7919ull + (uint64_t)g * 31ull + (s.minor ? 1 : 0));
    const auto& sets = s.minor ? kProgs[(size_t)g].minor : kProgs[(size_t)g].major;
    const auto& other = s.minor ? kProgs[(size_t)g].major : kProgs[(size_t)g].minor;
    const int vi = rng.below((int)sets.size());
    int ci = rng.below((int)sets.size() - 1);
    if (ci >= vi) ++ci;
    const Prog& verse = sets[(size_t)vi];
    const Prog& chorus = sets[(size_t)ci];
    const Prog& bridge = other[(size_t)rng.below((int)other.size())];
    Prog pre = { bridge[0], bridge[1], verse[0], verse[1] };

    Plan plan;
    auto S = [](const char* n, int b, const Prog& p, double e, int shift = 0) { Section x; x.name = n; x.bars = b; x.prog = p; x.energy = e; x.shift = shift; return x; };
    auto& v = plan.sections;
    const int loopBars = std::max(4, std::min(32, s.loopBars));

    if (!s.song)
    {
        if (s.complex)
        {
            const int half = std::max(2, loopBars / 2);
            v.push_back(S("Loop A", half, verse, 0.8));
            v.push_back(S("Loop B", loopBars - half, chorus, 0.9));
        }
        else
            v.push_back(S("Loop", loopBars, verse, 0.85));
    }
    else
    {
        const int target = std::max(16, (int)std::lround(std::max(1.0, std::min(5.0, s.minutes)) * std::max(40.0, s.bpm) / 4.0));
        if (!s.complex)
        {
            v.push_back(S("Intro", 4, verse, 0.35));
            int total = 4, i = 0;
            while (total + 4 + 8 <= target)
            {
                Section x = (i % 2 == 0) ? S("Verse", 8, verse, 0.6) : S("Chorus", 8, chorus, 0.9);
                if (i >= 2 && (i % 4 == 2 || i % 4 == 3)) x.name += " 2";
                v.push_back(x); total += 8; ++i;
            }
            bool anyChorus = false;
            for (auto& x : v) if (startsWith(x.name, "Chorus")) anyChorus = true;
            if (!anyChorus) v.push_back(S("Chorus", 8, chorus, 0.9));
            v.push_back(S("Outro", 4, chorus, 0.3));
        }
        else
        {
            const int keyUp = std::vector<int>{ 1, 2, 2, 0 }[(size_t)rng.below(4)];
            v = { S("Intro", 4, verse, 0.35), S("Verse", 8, verse, 0.6), S("Pre-Chorus", 4, pre, 0.7),
                  S("Chorus", 8, chorus, 0.95), S("Verse 2", 8, verse, 0.65), S("Pre-Chorus", 4, pre, 0.75),
                  S("Chorus", 8, chorus, 0.95), S("Bridge", 8, bridge, 0.7), S("Breakdown", 4, verse, 0.3),
                  S("Final Chorus", 8, chorus, 1.0, keyUp), S("Outro", 4, chorus, 0.35, keyUp) };
            auto total = [&]() { int t = 0; for (auto& x : v) t += x.bars; return t; };
            int extra = 0;
            while (total() + 16 <= target)
            {
                auto it = std::find_if(v.begin(), v.end(), [](const Section& x) { return x.name == "Bridge"; });
                std::string vn = "Verse " + std::to_string(3 + extra);
                Section vs = S("Verse", 8, verse, 0.65); vs.name = vn;
                it = v.insert(it, S("Chorus", 8, chorus, 0.95));
                v.insert(it, vs);
                ++extra;
            }
            const char* drops[] = { "Pre-Chorus", "Verse 2", "Breakdown", "Pre-Chorus", "Bridge" };
            for (const char* d : drops)
            {
                if (total() - target < 6) break;
                for (size_t k = 0; k < v.size(); ++k)
                {
                    if (v[k].name != d) continue;
                    v.erase(v.begin() + (long)k);
                    if (std::string(d) == "Verse 2")
                        for (size_t j = k; j < v.size(); ++j)
                            if (v[j].name == "Chorus") { v.erase(v.begin() + (long)j); break; }
                    break;
                }
            }
        }
    }

    int bar = 0;
    for (size_t si = 0; si < v.size(); ++si)
    {
        v[si].startBar = bar;
        for (int b = 0; b < v[si].bars; ++b)
        {
            const Chord& c = v[si].prog[(size_t)b % v[si].prog.size()];
            Bar x;
            x.section = (int)si; x.index = b;
            x.root = mod12(s.key + c.offset + v[si].shift);
            x.quality = c.quality;
            x.first = b == 0; x.last = b == v[si].bars - 1;
            plan.bars.push_back(x);
        }
        bar += v[si].bars;
    }
    return plan;
}

// ------------------------------------------------------------------------------------------------
std::vector<Note> generatePart(const Settings& s, const Plan& plan, int part)
{
    std::vector<Note> notes;
    const int g = std::max(0, std::min(kNumGenres - 1, s.genre));
    const Feel& feel = kFeel[g];
    Rng rng((uint64_t)s.songSeed * 100003ull + (uint64_t)s.partSeed[part] * 1009ull + (uint64_t)part * 17ull);
    Rng hr((uint64_t)s.partSeed[part] * 13ull + (uint64_t)part);
    Out out { notes, hr, s.humanize, part };
    const auto& bars = plan.bars;
    const auto& secs = plan.sections;
    const double dens = s.density;
    const bool cx = s.complex;
    const int type = s.partType[part];

    if (part == Keys)
    {
        std::string fam = type == 2 ? "held" : (type == 3 ? "chop" : feel.keys);
        const auto& cells = kKeysRhy.at(fam);
        std::vector<int> prev;
        std::map<int, const Cell*> cellBySec;
        for (size_t i = 0; i < bars.size(); ++i)
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Keys, sec, cx)) continue;
            if (!cellBySec.count(b.section)) cellBySec[b.section] = &rng.choice(cells);
            const Cell* cell = cellBySec[b.section];
            if (cx && b.index % 4 == 3 && fam != "held") cell = &rng.choice(cells);
            auto chord = voice(b.root, b.quality, prev, 62, 79);
            prev = chord;
            const double start = (double)i * 4;
            double vel = 0.45 + 0.3 * sec.energy;
            if (sec.name == "Outro") vel *= 1.0 - 0.6 * b.index / std::max(1, sec.bars);
            static const Cell held = { { 0, 4 } };
            if (b.last && sec.name == "Outro") cell = &held;
            for (size_t hk = 0; hk < cell->size(); ++hk)
            {
                const Hit& h = (*cell)[hk];
                if (dens < 0.3 && hk > 0 && h.s != 0 && h.s != 2 && fam != "held" && fam != "lush") continue;
                for (int p : chord) out.add(p, start + h.s, h.l, vel);
            }
            if (type == 1 && fam != "chop")
                out.add(36 + b.root + (b.root < 4 ? 12 : 0), start, fam == "held" ? 4 : 2, vel - 0.05);
        }
    }
    else if (part == Bass)
    {
        static const char* typeStyle[] = { "", "smooth", "octave", "walk", "808" };
        std::string style = type == 0 ? feel.bass : typeStyle[type];
        if (type == 1 && (std::string(feel.bass) == "slap" || std::string(feel.bass) == "motown")) style = feel.bass;
        const auto& cells = kBassRhy.at(style);
        const int low = style == "808" ? 28 : 31;
        for (size_t i = 0; i < bars.size(); ++i)
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Bass, sec, cx)) continue;
            const BassCell& cell = (cx || b.index == 0) ? rng.choice(cells) : (b.index % 2 == 0 ? cells.front() : cells.back());
            const int nxt = i + 1 < bars.size() ? bars[i + 1].root : b.root;
            const int rootp = low + mod12(b.root - low);
            const double start = (double)i * 4;
            const double vel = 0.65 + 0.25 * sec.energy;
            for (auto& h : cell)
            {
                const bool offbeat = std::fmod(h.s, 1.0) != 0.0;
                if (dens < 0.3 && offbeat) continue;
                int p;
                double l = h.l;
                if (h.d == 0) p = rootp;
                else if (h.d == 1) p = rootp + (std::strcmp(b.quality, "m7b5") == 0 ? 6 : 7);
                else if (h.d == 2) p = rootp + 12;
                else
                {
                    const int tgt = low + mod12(nxt - low);
                    static const int steps[4] = { -1, 1, 2, -2 };
                    p = tgt != rootp ? tgt + steps[rng.below(4)] : rootp + 7;
                }
                if (style == "walk" && h.d == 3) l = 1;
                out.add(p, start + h.s, l, vel - (offbeat ? 0.1 : 0.0));
            }
        }
    }
    else if (part == Lead)
    {
        static const char* typeKind[] = { "", "sax", "synth", "bells", "vocal" };
        const std::string kind = type == 0 ? feel.lead : typeKind[type];
        int lo = 64, hi = 88;
        if (kind == "sax") { lo = 58; hi = 82; }
        else if (kind == "bells") { lo = 72; hi = 94; }
        else if (kind == "vocal") { lo = 60; hi = 79; }
        const auto scale = scaleSet(s.key, s.minor, feel.dorian);
        const auto& cells = cx ? kMelComplex : kMelSimple;
        std::map<std::string, std::pair<Cell, std::vector<int>>> motifs;
        int last = (lo + hi) / 2;
        size_t i = 0;
        while (i < bars.size())
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Lead, sec, cx)) { ++i; continue; }
            const std::string tag = sec.name.substr(0, sec.name.find(' '));
            if (!motifs.count(tag))
            {
                Cell rhy = rng.choice(cells);
                if (dens < 0.35)
                {
                    Cell thin;
                    for (size_t k = 0; k < rhy.size(); ++k) if (k % 2 == 0 || rhy[k].l >= 1.5) thin.push_back(rhy[k]);
                    rhy = thin;
                }
                static const int cont[7] = { -2, -1, -1, 0, 1, 1, 2 };
                std::vector<int> contour;
                for (size_t k = 0; k < rhy.size(); ++k) contour.push_back(cont[rng.below(7)]);
                motifs[tag] = { rhy, contour };
            }
            const auto& rhy = motifs[tag].first;
            const auto& contour = motifs[tag].second;
            const bool answer = (b.index / 2) % 2 == 1;
            const int span = (sec.bars - b.index) >= 2 ? 2 : 1;
            const double vel = 0.6 + 0.3 * sec.energy;
            for (size_t k = 0; k < rhy.size(); ++k)
            {
                const double st = rhy[k].s, ln0 = rhy[k].l;
                if (st >= span * 4) continue;
                const size_t barK = i + (size_t)(st / 4);
                if (barK >= bars.size()) break;
                const Bar& bb = bars[barK];
                const auto ct = chordTones(bb.root, bb.quality);
                const bool strong = std::fmod(st, 1.0) == 0.0;
                int step = (answer && k + 3 >= rhy.size()) ? (cx ? -contour[k] : contour[k] - 1) : contour[k];
                if (cx && rng.uniform() < 0.15) step += rng.below(2) ? 1 : -1;
                std::vector<int> cands;
                for (int p = lo; p <= hi; ++p)
                {
                    const int pc = p % 12;
                    if (!(strong ? has(ct, pc) : has(scale, pc))) continue;
                    if (ln0 >= 1 && has(ct, mod12(pc - 1)) && !has(ct, pc)) continue;
                    cands.push_back(p);
                }
                if (cands.empty()) continue;
                const int want = last + step * 2;
                int best = cands[0]; double bd = 1e9;
                for (int c : cands) { const double d = std::abs(c - want) + rng.uniform(0, 1.5); if (d < bd) { bd = d; best = c; } }
                double ln = std::min(ln0, span * 4 - st);
                if (st + ln0 >= span * 4 - 0.01 && bb.last) ln = std::max(ln, 2.0);
                out.add(best, (double)i * 4 + st, ln, vel + (std::fmod(st, 2.0) == 0.0 ? 0.08 : 0.0));
                last = best;
            }
            if (cx && rng.uniform() < 0.4 && i + (size_t)span < bars.size())
            {
                const Bar& nb = bars[i + (size_t)span];
                const auto ct = chordTones(nb.root, nb.quality);
                int tgt = last, bd = 999;
                for (int p = lo; p <= hi; ++p) if (has(ct, p % 12) && std::abs(p - last) < bd) { bd = std::abs(p - last); tgt = p; }
                const int pick = has(scale, mod12(tgt - 1)) ? tgt - 1 : tgt - 2;
                out.add(pick, (double)(i + (size_t)span) * 4 - 0.5, 0.25, vel - 0.1);
            }
            i += (size_t)span;
        }
    }
    else if (part == Guitar)
    {
        std::vector<int> prev;
        for (size_t i = 0; i < bars.size(); ++i)
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Guitar, sec, cx)) continue;
            auto chord = voice(b.root, b.quality, prev, 67, 84);
            prev = chord;
            std::vector<int> sorted = chord;
            std::sort(sorted.begin(), sorted.end());
            std::vector<int> top3(sorted.end() - std::min<long>(3, (long)sorted.size()), sorted.end());
            const double start = (double)i * 4;
            const double vel = 0.5 + 0.25 * sec.energy;
            if (type == 0)
            {
                for (int k = 0; k < 16; ++k)
                {
                    if (!((k % 4 == 1 || k % 4 == 3) || (cx && rng.uniform() < 0.35 * dens))) continue;
                    const double st = k * 0.25;
                    for (int p : top3) out.add(p, start + st, 0.12, vel + (k % 4 == 1 ? 0.15 : 0.0));
                }
            }
            else if (type == 1)
            {
                std::vector<double> hits = { 1, 3 };
                if (cx) { hits.push_back(1.5); hits.push_back(3.5); }
                for (double st : hits) for (int p : top3) out.add(p, start + st, 0.3, vel);
            }
            else
            {
                const int n = (int)sorted.size();
                for (int k = 0; k < 8; ++k)
                {
                    const int idx = k < n ? k : std::max(0, 2 * n - k - 2);
                    out.add(sorted[(size_t)(idx % n)], start + k * 0.5, 0.45, vel - 0.05 * (k % 2));
                }
            }
        }
    }
    else if (part == Pad)
    {
        std::vector<int> prev;
        for (size_t i = 0; i < bars.size(); ++i)
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Pad, sec, cx)) continue;
            auto chord = voice(b.root, b.quality, prev, type == 1 ? 74 : 70, 90);
            prev = chord;
            for (int p : chord) out.add(p, (double)i * 4, 4.05, 0.35 + 0.2 * sec.energy);
        }
    }
    else if (part == Arp)
    {
        const double step = (cx && dens > 0.5) ? 0.25 : 0.5;
        for (size_t i = 0; i < bars.size(); ++i)
        {
            const Bar& b = bars[i];
            const Section& sec = secs[(size_t)b.section];
            if (!plays(Arp, sec, cx)) continue;
            std::vector<int> tones;
            for (int pc : chordTones(b.root, b.quality)) { const int p = 76 + mod12(pc - 76); if (!has(tones, p)) tones.push_back(p); }
            std::sort(tones.begin(), tones.end());
            const size_t n0 = tones.size();
            for (size_t k = 0; k < n0; ++k) tones.push_back(tones[k] + 12);
            std::vector<int> seq;
            if (type == 1) { seq = tones; for (size_t k = tones.size() - 2; k >= 1; --k) seq.push_back(tones[k]); }
            else if (type == 2) { for (size_t k = 0; k < tones.size(); k += 2) seq.push_back(tones[k]); for (size_t k = 1; k < tones.size(); k += 2) seq.push_back(tones[k]); }
            else seq = tones;
            const int n = (int)(4 / step);
            for (int k = 0; k < n; ++k)
                out.add(seq[(size_t)k % seq.size()], (double)i * 4 + k * step, step * 0.9, 0.45 + (k % 4 == 0 ? 0.2 : 0.0));
        }
    }

    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
    return notes;
}

} // namespace gb
