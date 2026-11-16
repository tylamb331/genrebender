#include "../Source/Generator.h"
#include <cstdio>
#include <cmath>
#include <cstring>
using namespace gb;
int main()
{
    long total = 0; int fails = 0; size_t biggest = 0;
    for (int g = 0; g < kNumGenres; ++g)
    for (int minor = 0; minor < 2; ++minor)
    for (int song = 0; song < 2; ++song)
    for (int cx = 0; cx < 2; ++cx)
    for (int part = 0; part < NumParts; ++part)
    for (int type = 0; type < (int)kPartTypes[(size_t)part].size(); ++type)
    {
        Settings s; s.genre = g; s.minor = minor; s.song = song; s.complex = cx; s.key = (g * 5) % 12;
        s.minutes = cx ? 5.0 : 1.0; s.bpm = cx ? 180 : 60; s.loopBars = cx ? 32 : 4;
        s.density = (g + part) % 2 ? 0.1 : 1.0; s.partType[part] = type; s.songSeed = 1 + g * 7 + type;
        Plan p = makePlan(s);
        auto ns = generatePart(s, p, part);
        if (ns.empty()) { printf("EMPTY %s %s song=%d cx=%d type=%d\n", kGenres[g], kPartNames[part], song, cx, type); fails++; continue; }
        const double endBeat = p.totalBars() * 4.0;
        for (auto& n : ns)
            if (n.pitch < 21 || n.pitch > 108 || n.start < 0 || n.length <= 0 || n.velocity <= 0 || n.velocity > 1 || n.start >= endBeat + 0.01)
            { printf("BAD note %s %s p=%d st=%.2f len=%.2f\n", kGenres[g], kPartNames[part], n.pitch, n.start, n.length); fails++; break; }
        if (song && cx)
        {
            const double mins = endBeat / s.bpm;
            if (std::fabs(mins - s.minutes) > 0.6) { printf("LENGTH %s %.2f min\n", kGenres[g], mins); fails++; }
        }
        total += (long)ns.size(); if (ns.size() > biggest) biggest = ns.size();
    }
    // determinism and part independence
    Settings s; s.genre = 3; s.minor = true; s.song = true; s.complex = true; s.minutes = 3; s.bpm = 92;
    Plan p = makePlan(s);
    auto a = generatePart(s, p, Lead), b = generatePart(s, p, Lead);
    if (a.size() != b.size()) fails++;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) if (a[i].pitch != b[i].pitch || a[i].start != b[i].start) { fails++; break; }
    auto keys1 = generatePart(s, p, Keys);
    s.partSeed[Lead] = 9;
    auto c = generatePart(s, p, Lead), keys2 = generatePart(s, p, Keys);
    bool leadChanged = c.size() != a.size(); for (size_t i = 0; !leadChanged && i < c.size(); ++i) if (c[i].pitch != a[i].pitch) leadChanged = true;
    bool keysSame = keys1.size() == keys2.size(); for (size_t i = 0; keysSame && i < keys1.size(); ++i) if (keys1[i].pitch != keys2[i].pitch || keys1[i].start != keys2[i].start) keysSame = false;
    if (!leadChanged || !keysSame) { printf("LOCK broken lead=%d keys=%d\n", leadChanged, keysSame); fails++; }
    // lead on strong beats sits on chord tones
    int strong = 0, off = 0;
    for (auto& n : a) { if (std::fmod(n.start, 1.0) != 0.0) continue; const Bar& bb = p.bars[(size_t)(n.start / 4)]; strong++;
        auto ct = chordTones(bb.root, bb.quality); bool in = false; for (int x : ct) if (x == n.pitch % 12) in = true; if (!in) off++; }
    printf("sections:"); for (auto& x : p.sections) printf(" %s(%d)", x.name.c_str(), x.bars); printf("\n");
    printf("lead strong-beat notes off the chord: %d of %d\n", off, strong);
    printf("%s: %ld notes across all combinations, biggest part %zu notes\n", fails ? "FAILED" : "ENGINE OK", total, biggest);
    return fails ? 1 : 0;
}
