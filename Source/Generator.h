// Genre Bender - generator engine (plain C++17, no JUCE), by Sir Lord Black.
// Same settings + seeds always give the same notes on every computer (own RNG, no std distributions).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace gb
{

enum Part { Keys = 0, Bass, Lead, Guitar, Pad, Arp, NumParts };

extern const char* const kPartNames[NumParts];
extern const char* const kGenres[];
extern const int kNumGenres;
extern const char* const kKeyNames[12];

// per-part "type" lists (index 0 is the first choice)
extern const std::vector<std::vector<std::string>> kPartTypes;

struct Settings
{
    int key = 0;              // 0 = C
    bool minor = false;
    int genre = 0;
    bool song = false;        // false = Loop mode
    bool complex = false;
    int loopBars = 8;         // 4..32
    double minutes = 2.5;     // 1..5 (Song mode)
    double bpm = 90.0;        // used to turn minutes into bars
    double density = 0.5;     // 0.1..1
    double humanize = 0.4;    // 0..1
    int songSeed = 1;
    int partSeed[NumParts] = { 1, 1, 1, 1, 1, 1 };
    int partType[NumParts] = { 0, 0, 0, 0, 0, 0 };
};

struct Note
{
    int pitch = 60;
    double start = 0;      // beats, on the grid
    double length = 1;     // beats
    float velocity = 0.8f; // 0..1
    float nudge = 0;       // humanize offset in beats (applied on playback/export)
    int part = 0;
};

struct Chord { int offset; const char* quality; };

struct Section
{
    std::string name;
    int bars = 4;
    std::vector<Chord> prog;
    double energy = 0.5;
    int shift = 0;          // key change in semitones
    int startBar = 0;
};

struct Bar
{
    int section = 0;        // index into plan sections
    int index = 0;          // bar number inside its section
    int root = 0;           // pitch class
    const char* quality = "maj7";
    bool first = false, last = false;
};

struct Plan
{
    std::vector<Section> sections;
    std::vector<Bar> bars;
    int totalBars() const { return (int)bars.size(); }
};

class Rng
{
public:
    explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) { next(); next(); }
    uint64_t next()
    {
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double uniform() { return (double)(next() >> 11) * (1.0 / 9007199254740992.0); }
    double uniform(double a, double b) { return a + (b - a) * uniform(); }
    int below(int n) { return n <= 1 ? 0 : (int)(next() % (uint64_t)n); }
    template <typename T> const T& choice(const std::vector<T>& v) { return v[(size_t)below((int)v.size())]; }
private:
    uint64_t s;
};

Plan makePlan(const Settings& s);
std::vector<Note> generatePart(const Settings& s, const Plan& plan, int part);
std::vector<int> chordTones(int root, const char* quality);

// Swing + humanize applied to a note's start (in beats) for playback / export.
double playStart(const Note& n, double swing);

} // namespace gb
