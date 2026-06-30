#include "waveform/renderers/waveformrenderlyrics.h"

#include <QDomNode>
#include <QPainter>

#include "control/controlproxy.h"
#include "track/track.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

WaveformRenderLyrics::WaveformRenderLyrics(
        WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererAbstract(waveformWidgetRenderer),
          m_font("sans-serif", 12),
          m_textColor(Qt::white),
          m_bgColor(0, 0, 0, 140),
          m_padding(6) {
}

WaveformRenderLyrics::~WaveformRenderLyrics() = default;

bool WaveformRenderLyrics::init() {
    m_pPlayPosition = std::make_unique<ControlProxy>(
            m_waveformRenderer->getGroup(), "playposition");
    m_pPlay = std::make_unique<ControlProxy>(
            m_waveformRenderer->getGroup(), "play");
    return true;
}

void WaveformRenderLyrics::setup(const QDomNode& node, const SkinContext& context) {
    // Read optional custom colors from skin XML
    const auto textColorStr = context.selectString(node, "LyricsTextColor");
    if (!textColorStr.isEmpty()) {
        m_textColor = QColor(textColorStr);
    }
    const auto bgColorStr = context.selectString(node, "LyricsBackgroundColor");
    if (!bgColorStr.isEmpty()) {
        m_bgColor = QColor(bgColorStr);
    }
    const auto fontSize = context.selectString(node, "LyricsFontSize");
    if (!fontSize.isEmpty()) {
        m_font.setPointSize(fontSize.toInt());
    }
}

void WaveformRenderLyrics::onSetTrack() {
    // Lyrics data is on the track, no action needed here
}

int WaveformRenderLyrics::findCurrentLineIndex(double currentTimeSeconds) const {
    const auto& track = m_waveformRenderer->getTrackInfo();
    if (!track) {
        return -1;
    }
    const auto& lyrics = track->getLyrics();
    if (lyrics.isEmpty()) {
        return -1;
    }

    // Find the last line whose timestamp is <= currentTimeSeconds
    int idx = -1;
    for (int i = 0; i < lyrics.size(); ++i) {
        if (lyrics[i].timestampSeconds <= currentTimeSeconds + 0.05) {
            idx = i;
        } else {
            break;
        }
    }
    return idx;
}

QString WaveformRenderLyrics::formatTime(double seconds) const {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void WaveformRenderLyrics::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(event);

    const auto& track = m_waveformRenderer->getTrackInfo();
    if (!track) {
        return;
    }

    const auto& lyrics = track->getLyrics();
    if (lyrics.isEmpty()) {
        return;
    }

    // Get current playback position
    double playPos = m_pPlayPosition->get();
    if (playPos < 0 || playPos > 1.0) {
        return;
    }

    double trackDuration = track->getDuration();
    double currentTimeSeconds = playPos * trackDuration;

    // Find the current lyrics line
    int currentLine = findCurrentLineIndex(currentTimeSeconds);
    if (currentLine < 0 || currentLine >= lyrics.size()) {
        return;
    }

    const auto& line = lyrics[currentLine];
    if (line.text.isEmpty()) {
        return;
    }

    // Find next timestamp for countdown
    double nextTimestamp = trackDuration;
    if (currentLine + 1 < lyrics.size()) {
        nextTimestamp = lyrics[currentLine + 1].timestampSeconds;
    }
    double timeUntilNext = nextTimestamp - currentTimeSeconds;
    if (timeUntilNext < 0) {
        timeUntilNext = 0;
    }

    // Prepare text
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    int widgetWidth = m_waveformRenderer->getWidth();
    int widgetHeight = m_waveformRenderer->getHeight();

    // Choose font size based on widget height
    int fontSize = qMax(10, widgetHeight / 18);
    m_font.setPointSize(fontSize);
    painter->setFont(m_font);

    // Build display text
    QString displayText = line.text;
    if (timeUntilNext > 0 && timeUntilNext < 60) {
        displayText += QString("  [%1]").arg(formatTime(timeUntilNext));
    }

    // Measure text
    QFontMetrics fm(m_font);
    int textWidth = fm.horizontalAdvance(displayText);
    int textHeight = fm.height();

    // Position: centered at bottom of waveform
    int boxWidth = textWidth + m_padding * 2;
    int boxHeight = textHeight + m_padding;
    int boxX = (widgetWidth - boxWidth) / 2;
    int boxY = widgetHeight - boxHeight - 4;

    // Ensure within bounds
    if (boxX < 0) {
        boxX = 2;
        boxWidth = widgetWidth - 4;
    }

    // Draw background
    painter->setBrush(m_bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(boxX, boxY, boxWidth, boxHeight, 4, 4);

    // Draw text
    painter->setPen(m_textColor);
    painter->drawText(
            boxX + m_padding,
            boxY + fm.ascent() + m_padding / 2,
            displayText);

    // Draw progress bar underneath (shows progress within current line)
    double lineDuration = nextTimestamp - line.timestampSeconds;
    if (lineDuration > 0) {
        double progress = (currentTimeSeconds - line.timestampSeconds) / lineDuration;
        progress = qBound(0.0, progress, 1.0);
        int barY = boxY + boxHeight;
        int barHeight = 3;
        painter->setBrush(QColor(255, 255, 255, 100));
        painter->drawRect(boxX, barY, boxWidth, barHeight);
        painter->setBrush(QColor(255, 255, 255, 200));
        painter->drawRect(boxX, barY, static_cast<int>(boxWidth * progress), barHeight);
    }

    painter->restore();
}
