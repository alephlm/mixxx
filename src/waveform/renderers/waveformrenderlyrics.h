#pragma once

#include <QColor>
#include <QFont>
#include <QString>

#include "audio/lrc.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class ControlProxy;
class QDomNode;
class SkinContext;

/// Renders a single line of LRC lyrics on the waveform, synchronized
/// to the current playback position.
class WaveformRenderLyrics : public WaveformRendererAbstract {
  public:
    explicit WaveformRenderLyrics(
            WaveformWidgetRenderer* waveformWidgetRenderer);
    ~WaveformRenderLyrics() override;

    bool init() override;
    void setup(const QDomNode& node, const SkinContext& context) override;
    void draw(QPainter* painter, QPaintEvent* event) override;
    void onSetTrack() override;

  private:
    /// Find the current lyrics line index for a given playback position in seconds
    int findCurrentLineIndex(double currentTimeSeconds) const;
    /// Format remaining time text
    QString formatTime(double seconds) const;

    std::unique_ptr<ControlProxy> m_pPlayPosition;
    std::unique_ptr<ControlProxy> m_pPlay;

    QFont m_font;
    QColor m_textColor;
    QColor m_bgColor;
    int m_padding;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderLyrics);
};
