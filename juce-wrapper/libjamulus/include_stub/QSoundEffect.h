/* Minimal QSoundEffect stub (header copy) to help static analysis detect language.
   This mirrors the existing stub file of the same name (no extension).
*/

#ifndef JUCE_WRAPPER_QSOUNDEFFECT_STUB_H
#define JUCE_WRAPPER_QSOUNDEFFECT_STUB_H

class QUrl; // forward-declare to avoid pulling Qt headers into this stub

class QSoundEffect
{
public:
    QSoundEffect() noexcept {}
    ~QSoundEffect() noexcept {}

    void setSource(const char* /*path*/) noexcept {}
    void setSource(const QUrl& /*url*/) noexcept {}

    void setVolume(float /*v*/) noexcept {}
    void play() noexcept {}
};

#endif // JUCE_WRAPPER_QSOUNDEFFECT_STUB_H

