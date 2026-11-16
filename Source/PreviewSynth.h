// Genre Bender - small built-in preview synth, so the plugin makes sound on its own.
// One simple voice recipe per part (keys, bass, lead, guitar, pad, arp). Not meant to replace your
// real instruments: route the MIDI out (or drag the MIDI) to those.
#pragma once
#include <cmath>
#include <cstring>

namespace gb
{

class PreviewSynth
{
public:
    static constexpr int kVoices = 64;

    void prepare(double sampleRate)
    {
        sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v.active = false;
        counter = 0;
    }

    void noteOn(int part, int pitch, float velocity)
    {
        Voice* pick = nullptr;
        for (auto& v : voices) if (!v.active) { pick = &v; break; }
        if (pick == nullptr)                         // steal the oldest
        {
            pick = &voices[0];
            for (auto& v : voices) if (v.age < pick->age) pick = &v;
        }
        Voice& v = *pick;
        v = Voice();
        v.active = true;
        v.part = part;
        v.pitch = pitch;
        v.vel = velocity;
        v.age = ++counter;
        v.freq = 440.0 * std::pow(2.0, (pitch - 69) / 12.0);
        v.detune[0] = 1.0; v.detune[1] = 1.0035; v.detune[2] = 0.9968;
        setEnvelope(v);
    }

    void noteOff(int part, int pitch)
    {
        for (auto& v : voices)
            if (v.active && !v.releasing && v.part == part && v.pitch == pitch) { v.releasing = true; }
    }

    void allOff()
    {
        for (auto& v : voices) if (v.active) v.releasing = true;
    }

    // adds into the buffers
    void render(float* left, float* right, int n, float gain)
    {
        const double twoPi = 6.283185307179586;
        for (auto& v : voices)
        {
            if (!v.active) continue;
            const double inc = v.freq / sr;
            for (int i = 0; i < n; ++i)
            {
                // envelope
                if (v.releasing) { v.env *= v.relMul; if (v.env < 1e-4) { v.active = false; break; } }
                else if (v.attacking) { v.env += v.atkInc; if (v.env >= 1.0) { v.env = 1.0; v.attacking = false; } }
                else { v.env = v.sustain + (v.env - v.sustain) * v.decMul; }
                double s = 0;
                const double p = v.phase[0];
                switch (v.part)
                {
                    case 0:   // electric piano: sine + soft second harmonic + a short tine
                        s = std::sin(twoPi * p) + 0.25 * std::sin(twoPi * 2 * p) + 0.18 * v.bright * std::sin(twoPi * 7 * p);
                        v.bright *= v.brightMul;
                        s *= 0.35;
                        break;
                    case 1:   // bass: round sine with a little grit
                        s = std::sin(twoPi * p) + 0.22 * std::sin(twoPi * 2 * p) + 0.08 * std::sin(twoPi * 3 * p);
                        s *= 0.55;
                        break;
                    case 2:   // lead: soft square with vibrato
                    {
                        const double vib = 1.0 + 0.004 * std::sin(twoPi * v.lfo) * std::fmin(1.0, v.t * 2.0);
                        v.lfo += 5.5 / sr;
                        const double q = p;
                        s = std::sin(twoPi * q) + std::sin(twoPi * 3 * q) / 3.0 + std::sin(twoPi * 5 * q) / 5.0 * 0.6;
                        s *= 0.3;
                        v.phase[0] += inc * (vib - 1.0);
                        break;
                    }
                    case 3:   // guitar: plucked, top harmonics die first
                        s = std::sin(twoPi * p) + 0.5 * v.bright * std::sin(twoPi * 2 * p) + 0.35 * v.bright * v.bright * std::sin(twoPi * 3 * p)
                            + 0.2 * v.bright * v.bright * v.bright * std::sin(twoPi * 4 * p);
                        v.bright *= v.brightMul;
                        s *= 0.3;
                        break;
                    case 4:   // pad: three detuned soft saws
                        for (int k = 0; k < 3; ++k)
                        {
                            const double pk = v.phase[k];
                            s += std::sin(twoPi * pk) + 0.5 * std::sin(twoPi * 2 * pk) + 0.25 * std::sin(twoPi * 3 * pk);
                            if (k > 0) { v.phase[k] += inc * v.detune[k]; if (v.phase[k] >= 1.0) v.phase[k] -= 1.0; }
                        }
                        s *= 0.12;
                        break;
                    default:  // bells: simple FM
                        s = std::sin(twoPi * p + 2.2 * v.bright * std::sin(twoPi * 3.5 * p));
                        v.bright *= v.brightMul;
                        s *= 0.28;
                        break;
                }
                v.phase[0] += inc;
                if (v.phase[0] >= 1.0) v.phase[0] -= 1.0;
                v.t += 1.0 / sr;
                const float out = (float)(s * v.env * v.vel) * gain;
                left[i] += out * v.panL;
                right[i] += out * v.panR;
            }
        }
    }

private:
    struct Voice
    {
        bool active = false, releasing = false, attacking = true;
        int part = 0, pitch = 60;
        float vel = 0.8f;
        unsigned age = 0;
        double freq = 440, phase[3] = { 0, 0.33, 0.66 }, detune[3] = { 1, 1, 1 };
        double env = 0, atkInc = 0.01, decMul = 0.9999, sustain = 0.7, relMul = 0.999;
        double bright = 1.0, brightMul = 0.9999, lfo = 0, t = 0;
        float panL = 0.7f, panR = 0.7f;
    };

    double msMul(double ms) const { return std::exp(-1.0 / (ms * 0.001 * sr)); }

    void setEnvelope(Voice& v)
    {
        double atk = 3, dec = 400, sus = 0.6, rel = 150, bright = 300;
        float pan = 0.0f;
        switch (v.part)
        {
            case 0: atk = 2;   dec = 900;  sus = 0.25; rel = 250; bright = 120; pan = -0.15f; break;
            case 1: atk = 4;   dec = 600;  sus = 0.7;  rel = 80;  break;
            case 2: atk = 12;  dec = 800;  sus = 0.75; rel = 180; pan = 0.1f; break;
            case 3: atk = 1;   dec = 250;  sus = 0.0;  rel = 90;  bright = 90; pan = 0.35f; break;
            case 4: atk = 450; dec = 1500; sus = 0.8;  rel = 900; pan = 0.0f; break;
            default: atk = 1;  dec = 700;  sus = 0.0;  rel = 400; bright = 250; pan = -0.35f; break;
        }
        v.atkInc = 1.0 / (atk * 0.001 * sr);
        v.decMul = msMul(dec);
        v.sustain = sus;
        v.relMul = msMul(rel);
        v.brightMul = msMul(bright);
        v.panL = (float)std::sqrt(0.5 * (1.0 - pan));
        v.panR = (float)std::sqrt(0.5 * (1.0 + pan));
    }

    Voice voices[kVoices];
    double sr = 44100.0;
    unsigned counter = 0;
};

} // namespace gb
