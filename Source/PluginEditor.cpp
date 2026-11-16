// Genre Bender - editor. See PluginEditor.h.
#include "PluginEditor.h"

using namespace gbui;

namespace
{
juce::FontOptions fopt(float size, bool bold = false, bool italic = false)
{
    int style = juce::Font::plain;
    if (bold) style |= juce::Font::bold;
    if (italic) style |= juce::Font::italic;
    return juce::FontOptions(size, style);
}

bool isBlack(int pitch)
{
    const int pc = pitch % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}
} // namespace

// ================================================================================================
// Look and feel
// ================================================================================================
VaporLook::VaporLook()
{
    setColour(juce::ComboBox::textColourId, kInk);
    setColour(juce::ComboBox::backgroundColourId, kPanel);
    setColour(juce::ComboBox::outlineColourId, kPurple.withAlpha(0.6f));
    setColour(juce::ComboBox::arrowColourId, kCyan);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a0f2c));
    setColour(juce::PopupMenu::textColourId, kInk);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kPink.withAlpha(0.35f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, kInk);
    setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    setColour(juce::Label::textColourId, kInk);
    setColour(juce::Slider::textBoxTextColourId, kInk);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ToggleButton::textColourId, kInk);
    setColour(juce::ScrollBar::thumbColourId, kPurple.withAlpha(0.7f));
    setColour(juce::ScrollBar::trackColourId, kPanel);
}

void VaporLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    const float size = (float)juce::jmin(w, h) - 6.0f;
    const auto c = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).getCentre();
    const float r = size * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    // neon value arc
    juce::Path track, value;
    track.addCentredArc(c.x, c.y, r, r, 0, a0, a1, true);
    value.addCentredArc(c.x, c.y, r, r, 0, a0, ang, true);
    g.setColour(juce::Colour(0xff2c1b45));
    g.strokePath(track, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(kPink.withAlpha(0.35f));
    g.strokePath(value, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(kPink);
    g.strokePath(value, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // chrome body
    const float kr = r - 7.0f;
    juce::ColourGradient chrome(juce::Colour(0xfff7f7fb), c.x - kr * 0.6f, c.y - kr * 0.8f,
                                juce::Colour(0xff6b6880), c.x + kr * 0.6f, c.y + kr, true);
    chrome.addColour(0.45, juce::Colour(0xffc9c6d6));
    chrome.addColour(0.7, juce::Colour(0xff8d89a3));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse(c.x - kr + 2, c.y - kr + 3, kr * 2, kr * 2);
    g.setGradientFill(chrome);
    g.fillEllipse(c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour(juce::Colour(0xff3a3550));
    g.drawEllipse(c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    // inner cap
    const float ir = kr * 0.62f;
    juce::ColourGradient cap(juce::Colour(0xffdcdae6), c.x, c.y - ir, juce::Colour(0xff7d7992), c.x, c.y + ir, false);
    g.setGradientFill(cap);
    g.fillEllipse(c.x - ir, c.y - ir, ir * 2, ir * 2);
    // pointer
    juce::Path ptr;
    ptr.addRoundedRectangle(-1.6f, -kr + 3.0f, 3.2f, kr * 0.55f, 1.5f);
    g.setColour(juce::Colour(0xff1b1028));
    g.fillPath(ptr, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
}

void VaporLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    const juce::Colour accent = b.findColour(juce::TextButton::buttonOnColourId);
    if (on)
    {
        g.setColour(accent.withAlpha(0.3f));
        g.fillRoundedRectangle(r.expanded(1.0f), 7.0f);
        g.setColour(accent);
        g.fillRoundedRectangle(r, 6.0f);
    }
    else
    {
        juce::ColourGradient grad(base.brighter(over ? 0.25f : 0.1f), r.getX(), r.getY(), base.darker(0.4f), r.getX(), r.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(accent.withAlpha(over ? 0.95f : 0.6f));
        g.drawRoundedRectangle(r, 6.0f, down ? 2.0f : 1.2f);
    }
}

void VaporLook::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    const juce::Colour accent = b.findColour(juce::ToggleButton::tickColourId);
    g.setColour(on ? accent.withAlpha(0.18f) : kPanel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(on ? accent : kPurple.withAlpha(over ? 0.9f : 0.5f));
    g.drawRoundedRectangle(r, 6.0f, on ? 1.8f : 1.1f);
    auto led = r.removeFromLeft(r.getHeight()).reduced(r.getHeight() * 0.3f);
    if (on) { g.setColour(accent.withAlpha(0.35f)); g.fillEllipse(led.expanded(3.0f)); }
    g.setColour(on ? accent : juce::Colour(0xff3d2a57));
    g.fillEllipse(led);
    g.setColour(on ? juce::Colours::white : kMuted);
    g.setFont(fopt(13.0f, true));
    g.drawText(b.getButtonText(), r.withTrimmedRight(4), juce::Justification::centredLeft);
}

void VaporLook::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(box.isMouseOver(true) ? kCyan : kPurple.withAlpha(0.6f));
    g.drawRoundedRectangle(r, 6.0f, 1.2f);
    juce::Path arrow;
    const float ax = (float)w - 16.0f, ay = (float)h * 0.5f;
    arrow.addTriangle(ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour(kCyan);
    g.fillPath(arrow);
}

juce::Font VaporLook::getComboBoxFont(juce::ComboBox&) { return fopt(14.0f, true); }
juce::Font VaporLook::getTextButtonFont(juce::TextButton&, int h) { return fopt(juce::jmin(15.0f, h * 0.5f), true); }

juce::Label* VaporLook::createSliderTextBox(juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setFont(fopt(12.5f, true));
    l->setColour(juce::Label::textColourId, kInk);
    l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

// ================================================================================================
// Knob
// ================================================================================================
Knob::Knob(const juce::String& n) : name(n)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible(slider);
}

void Knob::resized() { slider.setBounds(getLocalBounds().withTrimmedTop(16)); }

void Knob::paint(juce::Graphics& g)
{
    g.setColour(kCyan);
    g.setFont(fopt(11.5f, true));
    g.drawText(name, getLocalBounds().removeFromTop(16), juce::Justification::centred);
}

// ================================================================================================
// Drag button
// ================================================================================================
void DragMidiButton::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    const auto c = part < 0 ? kCyan : partColour(part);
    g.setColour(c.withAlpha(isMouseOver() ? 0.28f : 0.14f));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(c);
    g.drawRoundedRectangle(r, 6.0f, 1.4f);
    // grip dots
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 2; ++j)
            g.fillEllipse(r.getX() + 9.0f + j * 5.0f, r.getCentreY() - 6.0f + i * 5.0f, 2.6f, 2.6f);
    g.setColour(kInk);
    g.setFont(fopt(12.5f, true));
    g.drawText(label, r.withTrimmedLeft(24), juce::Justification::centredLeft);
}

void DragMidiButton::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 5) return;
    dragging = true;
    const auto file = proc.tempMidiFile(part);
    if (proc.writeMidiFile(file, part))
        juce::DragAndDropContainer::performExternalDragDropOfFiles({ file.getFullPathName() }, false, this);
}

// ================================================================================================
// Piano roll
// ================================================================================================
PianoRoll::PianoRoll(GenreBenderProcessor& p) : proc(p)
{
    addAndMakeVisible(hbar);
    hbar.addListener(this);
    hbar.setAutoHide(false);
    refresh();
}

void PianoRoll::refresh()
{
    for (int p = 0; p < gb::NumParts; ++p) notes[p] = proc.getPartNotes(p);
    plan = proc.getPlan();
    totalBeats = std::max(4.0, proc.getTotalBeats());
    zoomToFit();
}

juce::Rectangle<int> PianoRoll::gridArea() const
{
    return getLocalBounds().withTrimmedLeft(kKeysW).withTrimmedTop(kRulerH).withTrimmedBottom(kBarH);
}

float PianoRoll::xForBeat(double b) const { return (float)(gridArea().getX() + (b - scrollBeat) * pxPerBeat); }
double PianoRoll::beatForX(float x) const { return scrollBeat + (x - gridArea().getX()) / pxPerBeat; }
float PianoRoll::yForPitch(int p) const { return gridArea().getY() + (float)(topPitch - p) * rowH; }
int PianoRoll::pitchForY(float y) const { return topPitch - (int)std::floor((y - gridArea().getY()) / rowH); }

void PianoRoll::zoomToFit()
{
    const auto g = gridArea();
    if (g.getWidth() <= 0) return;
    pxPerBeat = juce::jlimit(1.5, 80.0, g.getWidth() / totalBeats);
    scrollBeat = 0;
    int lo = 127, hi = 0;
    for (int p = 0; p < gb::NumParts; ++p)
        if (activePart < 0 || p == activePart)
            for (auto& n : notes[p]) { lo = std::min(lo, n.pitch); hi = std::max(hi, n.pitch); }
    const int rows = std::max(12, (int)(g.getHeight() / rowH));
    if (hi < lo) { lo = 48; hi = 84; }
    const int mid = (lo + hi) / 2;
    topPitch = juce::jlimit(rows, 120, std::max(hi + 2, mid + rows / 2));
    resized();
    repaint();
}

void PianoRoll::resized()
{
    if (needsFit && gridArea().getWidth() > 0) { needsFit = false; zoomToFit(); return; }
    hbar.setBounds(getLocalBounds().removeFromBottom(kBarH).withTrimmedLeft(kKeysW));
    const auto g = gridArea();
    hbar.setRangeLimits(0.0, totalBeats);
    hbar.setCurrentRange(scrollBeat, std::min(totalBeats, g.getWidth() / pxPerBeat));
}

void PianoRoll::scrollBarMoved(juce::ScrollBar*, double start)
{
    scrollBeat = start;
    repaint();
}

void PianoRoll::tick()
{
    const double b = proc.getPlayBeat();
    if (std::abs(b - playBeat) > 1e-4)
    {
        playBeat = b;
        const auto g = gridArea();
        if (b >= 0 && (xForBeat(b) > g.getRight() - 20 || xForBeat(b) < g.getX()))
        {
            scrollBeat = std::max(0.0, std::min(b - 2.0, totalBeats - g.getWidth() / pxPerBeat));
            resized();
        }
        repaint();
    }
}

void PianoRoll::paint(juce::Graphics& g)
{
    const auto area = gridArea();
    g.fillAll(kBg);

    // rows
    const int bottomPitch = pitchForY((float)area.getBottom() - 0.01f);
    for (int p = topPitch; p >= bottomPitch; --p)
    {
        const float y = yForPitch(p);
        if (isBlack(p)) { g.setColour(juce::Colour(0xff0d0717)); g.fillRect((float)area.getX(), y, (float)area.getWidth(), rowH); }
        if (p % 12 == 0) { g.setColour(kPurple.withAlpha(0.25f)); g.drawHorizontalLine((int)(y + rowH), (float)area.getX(), (float)area.getRight()); }
    }

    // sections in the ruler, bar and beat lines
    juce::Rectangle<int> ruler(area.getX(), 0, area.getWidth(), kRulerH);
    g.setColour(juce::Colour(0xff1a0f2c));
    g.fillRect(ruler);
    int idx = 0;
    for (auto& s : plan.sections)
    {
        const float x0 = xForBeat(s.startBar * 4.0), x1 = xForBeat((s.startBar + s.bars) * 4.0);
        if (x1 < area.getX() || x0 > area.getRight()) { ++idx; continue; }
        auto band = juce::Rectangle<float>(x0, 0, x1 - x0, (float)kRulerH).getIntersection(ruler.toFloat());
        g.setColour((idx % 2 ? kPink : kCyan).withAlpha(0.16f));
        g.fillRect(band);
        g.setColour(idx % 2 ? kPink : kCyan);
        g.fillRect(juce::Rectangle<float>(std::max(x0, (float)area.getX()), kRulerH - 3.0f, std::max(0.0f, band.getWidth()), 3.0f));
        g.setColour(kInk);
        g.setFont(fopt(12.0f, true));
        juce::String label = juce::String(s.name) + (s.shift ? "  +" + juce::String(s.shift) : juce::String());
        g.drawText(label, band.reduced(5, 0).toNearestInt(), juce::Justification::centredLeft, true);
        ++idx;
    }
    const int bars = (int)std::ceil(totalBeats / 4.0);
    for (int b = 0; b <= bars; ++b)
    {
        const float x = xForBeat(b * 4.0);
        if (x < area.getX() - 1 || x > area.getRight() + 1) continue;
        g.setColour(kPurple.withAlpha(0.45f));
        g.drawVerticalLine((int)x, (float)area.getY(), (float)area.getBottom());
        if (pxPerBeat * 4 > 26 && b < bars)
        {
            g.setColour(kMuted);
            g.setFont(fopt(10.0f));
            g.drawText(juce::String(b + 1), (int)x + 2, area.getY(), 30, 12, juce::Justification::centredLeft);
        }
        if (pxPerBeat > 9 && b < bars)
            for (int k = 1; k < 4; ++k)
            {
                g.setColour(kPurple.withAlpha(0.15f));
                g.drawVerticalLine((int)xForBeat(b * 4.0 + k), (float)area.getY(), (float)area.getBottom());
            }
    }
    if (pxPerBeat > 36)
        for (double b = 0; b < totalBeats; b += 0.25)
            if (std::fmod(b, 1.0) != 0.0)
            {
                g.setColour(kPurple.withAlpha(0.07f));
                g.drawVerticalLine((int)xForBeat(b), (float)area.getY(), (float)area.getBottom());
            }

    // notes
    g.saveState();
    g.reduceClipRegion(area);
    for (int pass = 0; pass < 2; ++pass)
        for (int p = 0; p < gb::NumParts; ++p)
        {
            const bool dim = activePart >= 0 && p != activePart;
            if ((pass == 0) != dim) continue;        // dim parts first, the active one on top
            const auto col = partColour(p);
            for (auto& n : notes[p])
            {
                const float x0 = xForBeat(n.start), x1 = xForBeat(n.start + n.length);
                if (x1 < area.getX() || x0 > area.getRight()) continue;
                const float y = yForPitch(n.pitch);
                if (y + rowH < area.getY() || y > area.getBottom()) continue;
                juce::Rectangle<float> r(x0, y + 0.5f, std::max(2.0f, x1 - x0 - 1.0f), rowH - 1.0f);
                if (dim)
                {
                    g.setColour(col.withAlpha(0.16f));
                    g.fillRect(r);
                    continue;
                }
                g.setColour(col.withAlpha(0.25f));
                g.fillRoundedRectangle(r.expanded(1.5f), 3.0f);
                g.setColour(col.withMultipliedBrightness(0.55f + 0.45f * n.velocity));
                g.fillRoundedRectangle(r, 2.0f);
                g.setColour(col.brighter(0.6f));
                g.drawRoundedRectangle(r, 2.0f, 0.8f);
            }
        }
    // playhead
    if (playBeat >= 0)
    {
        const float x = xForBeat(playBeat);
        g.setColour(kYellow.withAlpha(0.3f));
        g.fillRect(x - 2.0f, (float)area.getY(), 5.0f, (float)area.getHeight());
        g.setColour(kYellow);
        g.drawVerticalLine((int)x, (float)area.getY(), (float)area.getBottom());
    }
    g.restoreState();

    // keyboard
    auto keys = getLocalBounds().withWidth(kKeysW).withTrimmedTop(kRulerH).withTrimmedBottom(kBarH);
    g.setColour(juce::Colour(0xffe9e2f5));
    g.fillRect(keys);
    for (int p = topPitch; p >= bottomPitch; --p)
    {
        const float y = yForPitch(p);
        if (y > keys.getBottom()) break;
        if (isBlack(p)) { g.setColour(juce::Colour(0xff1b1028)); g.fillRect((float)keys.getX(), y, kKeysW * 0.62f, rowH); }
        if (p % 12 == 0)
        {
            g.setColour(juce::Colour(0xff6b5a85));
            g.drawHorizontalLine((int)(y + rowH), (float)keys.getX(), (float)keys.getRight());
            g.setColour(juce::Colour(0xff2b1d40));
            g.setFont(fopt(9.5f, true));
            g.drawText("C" + juce::String(p / 12 - 1), keys.getX(), (int)y - 2, kKeysW - 3, (int)rowH + 3, juce::Justification::centredRight);
        }
    }
    g.setColour(juce::Colour(0xff1a0f2c));
    g.fillRect(0, 0, kKeysW, kRulerH);

    if (activePart < 0)
    {
        g.setColour(kMuted.withAlpha(0.85f));
        g.setFont(fopt(12.0f, false, true));
        g.drawText("Pick a part tab to edit its notes", area.reduced(8).removeFromBottom(18), juce::Justification::bottomRight);
    }
}

int PianoRoll::hitNote(juce::Point<float> pos, bool& onEdge) const
{
    onEdge = false;
    if (activePart < 0) return -1;
    const auto& v = notes[activePart];
    for (int i = (int)v.size() - 1; i >= 0; --i)
    {
        const auto& n = v[(size_t)i];
        const float x0 = xForBeat(n.start), x1 = xForBeat(n.start + n.length), y = yForPitch(n.pitch);
        if (pos.x >= x0 && pos.x <= x1 + 1 && pos.y >= y && pos.y < y + rowH)
        {
            onEdge = pos.x > x1 - std::min(6.0f, (x1 - x0) * 0.35f);
            return i;
        }
    }
    return -1;
}

void PianoRoll::mouseMove(const juce::MouseEvent& e)
{
    bool edge = false;
    const int i = hitNote(e.position, edge);
    setMouseCursor(i >= 0 ? (edge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::DraggingHandCursor)
                          : juce::MouseCursor::NormalCursor);
}

void PianoRoll::mouseDown(const juce::MouseEvent& e)
{
    drag = None;
    if (activePart < 0 || !gridArea().contains(e.getPosition())) return;
    bool edge = false;
    const int i = hitNote(e.position, edge);
    auto& v = notes[activePart];
    if (e.mods.isPopupMenu())                       // right-click deletes
    {
        if (i >= 0) { v.erase(v.begin() + i); commit(); repaint(); }
        return;
    }
    dragStart = e.position;
    if (i >= 0)
    {
        dragIndex = i;
        dragOrig = v[(size_t)i];
        drag = edge ? Resize : Move;
        lastLength = dragOrig.length;
        return;
    }
    gb::Note n;
    n.part = activePart;
    n.pitch = juce::jlimit(0, 127, pitchForY(e.position.y));
    n.start = std::max(0.0, std::floor(beatForX(e.position.x) / snap) * snap);
    n.length = std::max(snap, lastLength);
    n.velocity = 0.8f;
    n.nudge = 0.0f;
    if (n.start >= totalBeats) return;
    v.push_back(n);
    dragIndex = (int)v.size() - 1;
    dragOrig = n;
    drag = Create;
    repaint();
}

void PianoRoll::mouseDrag(const juce::MouseEvent& e)
{
    if (drag == None || activePart < 0) return;
    auto& v = notes[activePart];
    if (dragIndex < 0 || dragIndex >= (int)v.size()) return;
    auto& n = v[(size_t)dragIndex];
    if (drag == Move)
    {
        const double db = (e.position.x - dragStart.x) / pxPerBeat;
        const int dp = (int)std::lround((dragStart.y - e.position.y) / rowH);
        n.start = juce::jlimit(0.0, totalBeats - snap, std::round((dragOrig.start + db) / snap) * snap);
        n.pitch = juce::jlimit(0, 127, dragOrig.pitch + dp);
    }
    else
    {
        const double end = std::round(beatForX(e.position.x) / snap) * snap;
        n.length = juce::jlimit(snap, totalBeats - n.start, end - n.start);
        lastLength = n.length;
    }
    repaint();
}

void PianoRoll::mouseUp(const juce::MouseEvent&)
{
    if (drag != None) commit();
    drag = None;
}

void PianoRoll::mouseDoubleClick(const juce::MouseEvent& e)
{
    bool edge = false;
    const int i = hitNote(e.position, edge);
    if (i >= 0 && activePart >= 0)
    {
        notes[activePart].erase(notes[activePart].begin() + i);
        drag = None;
        commit();
        repaint();
    }
}

void PianoRoll::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto g = gridArea();
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())        // zoom around the mouse
    {
        const double at = beatForX(e.position.x);
        pxPerBeat = juce::jlimit(1.5, 120.0, pxPerBeat * (w.deltaY > 0 ? 1.2 : 1.0 / 1.2));
        scrollBeat = juce::jlimit(0.0, std::max(0.0, totalBeats - g.getWidth() / pxPerBeat), at - (e.position.x - g.getX()) / pxPerBeat);
    }
    else if (e.mods.isShiftDown() || std::abs(w.deltaX) > std::abs(w.deltaY))
    {
        const double d = (std::abs(w.deltaX) > std::abs(w.deltaY) ? w.deltaX : w.deltaY) * 200.0 / pxPerBeat;
        scrollBeat = juce::jlimit(0.0, std::max(0.0, totalBeats - g.getWidth() / pxPerBeat), scrollBeat - d);
    }
    else
    {
        const int rows = (int)(g.getHeight() / rowH);
        topPitch = juce::jlimit(rows, 127, topPitch + (w.deltaY > 0 ? 3 : -3));
    }
    resized();
    repaint();
}

void PianoRoll::commit()
{
    if (activePart >= 0) proc.setPartNotes(activePart, notes[activePart]);
}

// ================================================================================================
// Editor
// ================================================================================================
GenreBenderEditor::GenreBenderEditor(GenreBenderProcessor& p)
    : AudioProcessorEditor(&p), proc(p), dragPart(p, "DRAG PART"), roll(p), dragAll(p, "DRAG FULL BAND")
{
    setLookAndFeel(&look);
    auto& st = proc.apvts;

    auto setupCombo = [this](juce::ComboBox& c, const juce::StringArray& items) {
        c.addItemList(items, 1);
        addAndMakeVisible(c);
    };
    juce::StringArray genres, keys;
    for (int i = 0; i < gb::kNumGenres; ++i) genres.add(gb::kGenres[i]);
    for (auto* k : gb::kKeyNames) keys.add(k);
    setupCombo(genre, genres);
    setupCombo(key, keys);
    setupCombo(tonality, { "Major", "Minor" });
    setupCombo(mode, { "Loop", "Song" });
    setupCombo(complexity, { "Simple", "Complex" });
    aGenre = std::make_unique<CA>(st, "genre", genre);
    aKey = std::make_unique<CA>(st, "key", key);
    aTon = std::make_unique<CA>(st, "tonality", tonality);
    aMode = std::make_unique<CA>(st, "mode", mode);
    aCx = std::make_unique<CA>(st, "complexity", complexity);
    mode.onChange = [this] { updateModeVisibility(); };

    for (auto* k : { &loopBars, &minutes, &density, &swing, &humanize, &volume }) addAndMakeVisible(*k);
    aLoop = std::make_unique<SA>(st, "loopBars", loopBars.slider);
    aMin = std::make_unique<SA>(st, "minutes", minutes.slider);
    aDen = std::make_unique<SA>(st, "density", density.slider);
    aSwing = std::make_unique<SA>(st, "swing", swing.slider);
    aHum = std::make_unique<SA>(st, "humanize", humanize.slider);
    aVol = std::make_unique<SA>(st, "volume", volume.slider);
    volume.slider.setTextValueSuffix(" dB");

    songSeed.setSliderStyle(juce::Slider::IncDecButtons);
    songSeed.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 64, 30);
    songSeed.setColour(juce::TextButton::buttonColourId, kPanel);
    songSeed.setColour(juce::Slider::textBoxBackgroundColourId, kPanel);
    songSeed.setColour(juce::Slider::textBoxOutlineColourId, kPurple.withAlpha(0.6f));
    addAndMakeVisible(songSeed);
    aSeed = std::make_unique<SA>(st, "songSeed", songSeed);

    dice.setColour(juce::TextButton::buttonColourId, kPanel);
    dice.setColour(juce::TextButton::buttonOnColourId, kCyan);
    dice.onClick = [this] { proc.newSongSeed(); };
    addAndMakeVisible(dice);
    generate.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff5b1f6e));
    generate.setColour(juce::TextButton::buttonOnColourId, kPink);
    generate.onClick = [this] { proc.generateAll(); };
    addAndMakeVisible(generate);

    for (int t = 0; t <= gb::NumParts; ++t)
    {
        auto& b = tabs[t];
        b.setButtonText(t == 0 ? "ALL" : juce::String(gb::kPartNames[t - 1]).toUpperCase());
        b.setColour(juce::TextButton::buttonColourId, kPanel);
        b.setColour(juce::TextButton::buttonOnColourId, t == 0 ? kInk : partColour(t - 1));
        b.onClick = [this, t] { selectPart(t - 1); };
        addAndMakeVisible(b);
    }
    addAndMakeVisible(partType);
    partSeedLabel.setJustificationType(juce::Justification::centred);
    partSeedLabel.setFont(fopt(13.0f, true));
    addAndMakeVisible(partSeedLabel);
    reroll.setColour(juce::TextButton::buttonColourId, kPanel);
    reroll.setColour(juce::TextButton::buttonOnColourId, kMint);
    reroll.onClick = [this] { if (activePart >= 0) proc.rerollPart(activePart); };
    addAndMakeVisible(reroll);
    lock.setColour(juce::ToggleButton::tickColourId, kYellow);
    mute.setColour(juce::ToggleButton::tickColourId, kOrange);
    addAndMakeVisible(lock);
    addAndMakeVisible(mute);
    addAndMakeVisible(dragPart);
    snapBox.addItemList({ "Snap 1/4", "Snap 1/8", "Snap 1/16", "Snap 1/32" }, 1);
    snapBox.setSelectedId(3, juce::dontSendNotification);
    snapBox.onChange = [this] {
        static const double s[4] = { 1.0, 0.5, 0.25, 0.125 };
        roll.setSnap(s[juce::jlimit(0, 3, snapBox.getSelectedId() - 1)]);
    };
    addAndMakeVisible(snapBox);

    addAndMakeVisible(roll);

    play.setColour(juce::TextButton::buttonColourId, kPanel);
    play.setColour(juce::TextButton::buttonOnColourId, kYellow);
    play.onClick = [this] { proc.setPreviewPlaying(!proc.isPreviewPlaying()); };
    addAndMakeVisible(play);
    previewSound.setColour(juce::ToggleButton::tickColourId, kPink);
    midiOut.setColour(juce::ToggleButton::tickColourId, kCyan);
    addAndMakeVisible(previewSound);
    addAndMakeVisible(midiOut);
    aPrev = std::make_unique<BA>(st, "preview", previewSound);
    aMidi = std::make_unique<BA>(st, "midiOut", midiOut);
    fit.setColour(juce::TextButton::buttonColourId, kPanel);
    fit.setColour(juce::TextButton::buttonOnColourId, kCyan);
    fit.onClick = [this] { roll.zoomToFit(); };
    addAndMakeVisible(fit);
    exportBtn.setColour(juce::TextButton::buttonColourId, kPanel);
    exportBtn.setColour(juce::TextButton::buttonOnColourId, kCyan);
    exportBtn.onClick = [this] {
        auto dir = juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Genre Bender");
        dir.createDirectory();
        auto f = dir.getChildFile(proc.tempMidiFile(activePart).getFileName());
        if (proc.writeMidiFile(f, activePart)) f.revealToUser();
    };
    addAndMakeVisible(exportBtn);
    addAndMakeVisible(dragAll);
    status.setFont(fopt(12.5f, true));
    status.setColour(juce::Label::textColourId, kCyan);
    status.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(status);

    proc.addChangeListener(this);
    selectPart(-1);
    updateModeVisibility();
    status.setText(proc.describe(), juce::dontSendNotification);
    setSize(1180, 760);
    startTimerHz(30);
}

GenreBenderEditor::~GenreBenderEditor()
{
    proc.removeChangeListener(this);
    aType.reset(); aLock.reset(); aMute.reset();
    setLookAndFeel(nullptr);
}

void GenreBenderEditor::selectPart(int p)
{
    activePart = p;
    for (int t = 0; t <= gb::NumParts; ++t) tabs[t].setToggleState(t - 1 == p, juce::dontSendNotification);
    aType.reset(); aLock.reset(); aMute.reset();
    const bool part = p >= 0;
    for (juce::Component* c : { (juce::Component*)&partType, (juce::Component*)&partSeedLabel, (juce::Component*)&reroll,
                                (juce::Component*)&lock, (juce::Component*)&mute })
        c->setVisible(part);
    if (part)
    {
        partType.clear(juce::dontSendNotification);
        juce::StringArray items;
        for (auto& s : gb::kPartTypes[(size_t)p]) items.add(s);
        partType.addItemList(items, 1);
        aType = std::make_unique<CA>(proc.apvts, GenreBenderProcessor::partParam("type", p), partType);
        aLock = std::make_unique<BA>(proc.apvts, GenreBenderProcessor::partParam("lock", p), lock);
        aMute = std::make_unique<BA>(proc.apvts, GenreBenderProcessor::partParam("mute", p), mute);
        reroll.setColour(juce::TextButton::buttonOnColourId, partColour(p));
    }
    dragPart.setPart(p);
    roll.setActivePart(p);
    roll.zoomToFit();
    timerCallback();
    repaint();
}

void GenreBenderEditor::updateModeVisibility()
{
    const bool song = mode.getSelectedItemIndex() == 1;
    loopBars.setVisible(!song);
    minutes.setVisible(song);
}

void GenreBenderEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    roll.refresh();
    status.setText(proc.describe(), juce::dontSendNotification);
}

void GenreBenderEditor::timerCallback()
{
    roll.tick();
    const bool pl = proc.isPreviewPlaying();
    play.setButtonText(pl ? "STOP" : "PLAY");
    play.setToggleState(pl, juce::dontSendNotification);
    if (activePart >= 0)
    {
        const int s = (int)proc.apvts.getRawParameterValue(GenreBenderProcessor::partParam("seed", activePart))->load();
        partSeedLabel.setText("seed " + juce::String(s), juce::dontSendNotification);
        partSeedLabel.setColour(juce::Label::textColourId, partColour(activePart));
    }
}

void GenreBenderEditor::paint(juce::Graphics& g)
{
    const float W = (float)getWidth();
    g.fillAll(kBg);

    // ---- header: sunset sky, striped sun, horizon ----
    const float hh = 100.0f;
    juce::ColourGradient sky(juce::Colour(0xff12062a), 0, 0, juce::Colour(0xff7a1f6e), 0, hh, false);
    sky.addColour(0.65, juce::Colour(0xff3f1260));
    g.setGradientFill(sky);
    g.fillRect(0.0f, 0.0f, W, hh);
    // stars
    juce::Random rnd(7);
    for (int i = 0; i < 60; ++i)
    {
        g.setColour(juce::Colours::white.withAlpha(0.2f + 0.5f * rnd.nextFloat()));
        g.fillEllipse(rnd.nextFloat() * W, rnd.nextFloat() * hh * 0.6f, 1.6f, 1.6f);
    }
    // sun
    const float sx = W - 250.0f, sy = hh + 6.0f, sr = 86.0f;
    g.saveState();
    g.reduceClipRegion(0, 0, (int)W, (int)hh);
    juce::ColourGradient sun(juce::Colour(0xfffff36b), sx, sy - sr, juce::Colour(0xffff3fa4), sx, sy + sr * 0.2f, false);
    g.setColour(juce::Colour(0xffff71ce).withAlpha(0.25f));
    g.fillEllipse(sx - sr - 14, sy - sr - 14, (sr + 14) * 2, (sr + 14) * 2);
    g.setGradientFill(sun);
    g.fillEllipse(sx - sr, sy - sr, sr * 2, sr * 2);
    for (int i = 0; i < 6; ++i)                     // the classic stripes
    {
        const float y = sy - sr * 0.55f + i * 10.0f;
        g.setColour(juce::Colour(0xff6a1b68));
        g.fillRect(sx - sr, y, sr * 2, 2.0f + i * 1.2f);
    }
    g.restoreState();
    g.setColour(kPink);
    g.fillRect(0.0f, hh - 2.0f, W, 2.0f);

    // title
    const juce::String title = "GENRE BENDER";
    g.setFont(fopt(46.0f, true, true));
    for (int i = 4; i >= 1; --i)
    {
        g.setColour(kPink.withAlpha(0.12f));
        g.drawText(title, 22 - i, 14 - i, 520, 56, juce::Justification::centredLeft);
        g.drawText(title, 22 + i, 14 + i, 520, 56, juce::Justification::centredLeft);
    }
    juce::GlyphArrangement ga;
    ga.addLineOfText(g.getCurrentFont(), title, 22.0f, 58.0f);
    juce::Path tp;
    ga.createPath(tp);
    juce::ColourGradient chrome(juce::Colour(0xff9ff6ff), 0, 22, juce::Colour(0xffff8ad8), 0, 62, false);
    chrome.addColour(0.5, juce::Colours::white);
    chrome.addColour(0.52, juce::Colour(0xff5ad9ff));
    g.setGradientFill(chrome);
    g.fillPath(tp);
    g.setColour(juce::Colour(0xff2a0b3a));
    g.strokePath(tp, juce::PathStrokeType(1.2f));
    g.setColour(kCyan);
    g.setFont(fopt(13.0f, true));
    g.drawText("by  SIR LORD BLACK", 26, 66, 300, 20, juce::Justification::centredLeft);

    // ---- left panel ----
    auto panel = juce::Rectangle<float>(10.0f, 108.0f, 282.0f, (float)getHeight() - 118.0f);
    g.setColour(kPanel.withAlpha(0.85f));
    g.fillRoundedRectangle(panel, 10.0f);
    g.setColour(kPurple.withAlpha(0.35f));
    g.drawRoundedRectangle(panel, 10.0f, 1.0f);
    // perspective grid floor in the bottom of the panel
    {
        const float top = 612.0f, bottom = panel.getBottom() - 6.0f, cx = panel.getCentreX();
        g.saveState();
        g.reduceClipRegion(panel.toNearestInt().withTop((int)top));
        for (int i = -10; i <= 10; ++i)
        {
            g.setColour(kCyan.withAlpha(0.28f));
            g.drawLine(cx + i * 6.0f, top, cx + i * 60.0f, bottom, 1.0f);
        }
        for (int k = 0; k < 7; ++k)
        {
            const float t = std::pow(k / 6.0f, 1.8f);
            const float y = top + t * (bottom - top);
            g.setColour(kCyan.withAlpha(0.15f + 0.25f * t));
            g.drawHorizontalLine((int)y, panel.getX(), panel.getRight());
        }
        g.restoreState();
    }
    // captions above the combos
    g.setColour(kCyan);
    g.setFont(fopt(11.5f, true));
    auto cap = [&](juce::Component& c, const char* t) { g.drawText(t, c.getX(), c.getY() - 16, c.getWidth(), 14, juce::Justification::centredLeft); };
    cap(genre, "GENRE");
    cap(key, "KEY");
    cap(tonality, "TONALITY");
    cap(mode, "MODE");
    cap(complexity, "COMPLEXITY");
    cap(songSeed, "SONG SEED");
}

void GenreBenderEditor::resized()
{
    const int lx = 22, lw = 258;
    genre.setBounds(lx, 132, lw, 30);
    key.setBounds(lx, 186, 124, 30);
    tonality.setBounds(lx + 134, 186, 124, 30);
    mode.setBounds(lx, 240, 124, 30);
    complexity.setBounds(lx + 134, 240, 124, 30);
    loopBars.setBounds(lx, 282, 84, 96);
    minutes.setBounds(lx, 282, 84, 96);
    density.setBounds(lx + 87, 282, 84, 96);
    swing.setBounds(lx + 174, 282, 84, 96);
    humanize.setBounds(lx, 384, 84, 96);
    volume.setBounds(lx + 87, 384, 84, 96);
    songSeed.setBounds(lx, 506, 160, 30);
    dice.setBounds(lx + 168, 506, 90, 30);
    generate.setBounds(lx, 548, lw, 54);

    const int rx = 304, rw = getWidth() - rx - 12;
    const int tw = rw / (gb::NumParts + 1);
    for (int t = 0; t <= gb::NumParts; ++t) tabs[t].setBounds(rx + t * tw, 110, tw - 6, 30);
    int x = rx;
    const int py = 148, ph = 32;
    partType.setBounds(x, py, 200, ph); x += 208;
    partSeedLabel.setBounds(x, py, 80, ph); x += 84;
    reroll.setBounds(x, py, 90, ph); x += 98;
    lock.setBounds(x, py, 82, ph); x += 90;
    mute.setBounds(x, py, 82, ph); x += 90;
    dragPart.setBounds(x, py, 132, ph); x += 140;
    snapBox.setBounds(rx + rw - 110, py, 110, ph);

    roll.setBounds(rx, 188, rw, getHeight() - 188 - 58);

    const int by = getHeight() - 48;
    int bx = rx;
    play.setBounds(bx, by, 90, 36); bx += 98;
    previewSound.setBounds(bx, by, 150, 36); bx += 158;
    midiOut.setBounds(bx, by, 110, 36); bx += 118;
    fit.setBounds(bx, by, 60, 36); bx += 68;
    exportBtn.setBounds(bx, by, 120, 36); bx += 128;
    dragAll.setBounds(bx, by, 160, 36); bx += 168;
    status.setBounds(330, 70, 500, 22);
}
