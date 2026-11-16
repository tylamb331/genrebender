# Genre Bender

Multi-genre MIDI generator plugin (VST3 / AU) by **Sir Lord Black**.

- Genres: Vaporwave, City Pop, Future Funk, 90s R&B, Boom Bap Soul, Smooth Jazz, Lounge / Elevator, 80s R&B, Motown
- Loop mode (4-32 bars) or Song mode (1-5 minutes, intro / verses / choruses / bridge / breakdown / outro)
- Six parts on their own MIDI channels: Keys, Bass, Lead, Guitar, Pad, Arp, each with its own type, seed, lock and mute
- Editable piano roll per part, built-in preview sound, plays in sync with the host
- Drag any part (or the full band) straight into FL Studio as MIDI, or export MIDI files

Mac builds are made automatically by GitHub Actions on every push: see **Releases > latest > Genre-Bender-Mac.zip**,
and `INSTALL-MAC.txt` inside it.

Build yourself: `cmake -B build -G Xcode && cmake --build build --config Release --target GenreBender_All`
(JUCE 8 is downloaded automatically).
