#include "waveform/renderers/allshader/waveformrenderlyrics.h"

#include <QDomNode>
#include <QFontMetrics>
#include <QPainter>

#include "control/controlproxy.h"
#include "moc_waveformrenderlyrics.cpp"
#include "rendergraph/context.h"
#include "rendergraph/geometry.h"
#include "rendergraph/material/texturematerial.h"
#include "rendergraph/texture.h"
#include "rendergraph/vertexupdaters/texturedvertexupdater.h"
#include "skin/legacy/skincontext.h"
#include "track/track.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

using namespace rendergraph;

namespace allshader {

WaveformRenderLyrics::WaveformRenderLyrics(
        WaveformWidgetRenderer* waveformWidget,
        ::WaveformRendererAbstract::PositionSource type)
        : ::WaveformRendererAbstract(waveformWidget),
          m_textColor(Qt::white),
          m_bgColor(0, 0, 0, 140),
          m_padding(6),
          m_isSlipRenderer(type == ::WaveformRendererAbstract::Slip) {
    initForRectangles<TextureMaterial>(1);
    setUsePreprocess(true);
}

WaveformRenderLyrics::~WaveformRenderLyrics() = default;

bool WaveformRenderLyrics::init() {
    m_pPlayPosition = std::make_unique<ControlProxy>(
            m_waveformRenderer->getGroup(), "playposition");
    m_pPlay = std::make_unique<ControlProxy>(
            m_waveformRenderer->getGroup(), "play");
    return true;
}

void WaveformRenderLyrics::setup(
        const QDomNode& node, const SkinContext& context) {
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
    m_cache.lastLineIndex = -1;
    m_cache.text.clear();
}

void WaveformRenderLyrics::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
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

void WaveformRenderLyrics::updateTexture() {
    const auto& track = m_waveformRenderer->getTrackInfo();
    if (!track) {
        clearGeometry();
        return;
    }

    const auto& lyrics = track->getLyrics();
    if (lyrics.isEmpty()) {
        clearGeometry();
        return;
    }

    double playPos = m_pPlayPosition->get();
    if (playPos < 0 || playPos > 1.0) {
        clearGeometry();
        return;
    }

    double trackDuration = track->getDuration();
    double currentTimeSeconds = playPos * trackDuration;

    int currentLine = findCurrentLineIndex(currentTimeSeconds);
    if (currentLine < 0) {
        clearGeometry();
        return;
    }

    const auto& line = lyrics[currentLine];
    if (line.text.isEmpty()) {
        clearGeometry();
        return;
    }

    // Build display text
    QString displayText = line.text;
    double nextTimestamp = trackDuration;
    if (currentLine + 1 < lyrics.size()) {
        nextTimestamp = lyrics[currentLine + 1].timestampSeconds;
    }
    double timeUntilNext = nextTimestamp - currentTimeSeconds;
    if (timeUntilNext > 0 && timeUntilNext < 60) {
        displayText += QString("  [%1]").arg(formatTime(timeUntilNext));
    }

    // Recreate texture only if the text changed
    if (m_cache.lastLineIndex != currentLine || m_cache.text != line.text) {
        m_cache.lastLineIndex = currentLine;
        m_cache.text = line.text;

        int widgetHeight = m_waveformRenderer->getHeight();
        float devicePixelRatio = m_waveformRenderer->getDevicePixelRatio();

        int fontSize = qMax(10, widgetHeight / 18);
        QFont font("sans-serif", fontSize);
        QFontMetrics fm(font);

        int textWidth = fm.horizontalAdvance(displayText);
        int textHeight = fm.height();
        int boxWidth = textWidth + m_padding * 2;
        int boxHeight = textHeight + m_padding;

        int imageW = static_cast<int>(boxWidth * devicePixelRatio + 0.5f);
        int imageH = static_cast<int>((boxHeight + 4) * devicePixelRatio + 0.5f);
        if (imageW <= 0 || imageH <= 0)
            return;

        QImage image(imageW, imageH, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(devicePixelRatio);
        image.fill(Qt::transparent);

        {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setFont(font);

            // Background
            painter.setBrush(m_bgColor);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(0, 0, boxWidth, boxHeight + 4, 4, 4);

            // Text
            painter.setPen(m_textColor);
            painter.drawText(m_padding, fm.ascent() + m_padding / 2, displayText);

            painter.end();
        }

        auto* pContext = m_waveformRenderer->getContext();
        dynamic_cast<TextureMaterial&>(material())
                .setTexture(std::make_unique<Texture>(pContext, image));
        m_cache.bBoxWidth = boxWidth;
        m_cache.bBoxHeight = boxHeight + 4;
    }

    // Update geometry
    int widgetWidth = m_waveformRenderer->getWidth();
    int widgetHeight = m_waveformRenderer->getHeight();
    if (widgetWidth <= 0 || widgetHeight <= 0)
        return;

    geometry().allocate(6);

    float x = (widgetWidth - m_cache.bBoxWidth) / 2.0f;
    float y = widgetHeight - m_cache.bBoxHeight - 4.0f;
    if (x < 0)
        x = 2;

    TexturedVertexUpdater vertexUpdater{
            geometry().vertexDataAs<Geometry::TexturedPoint2D>()};
    vertexUpdater.addRectangle(
            {x, y},
            {x + static_cast<float>(m_cache.bBoxWidth),
                    y + static_cast<float>(m_cache.bBoxHeight)},
            {0.f, 0.f},
            {1.f, 1.f});

    markDirtyGeometry();
    markDirtyMaterial();
}

void WaveformRenderLyrics::clearGeometry() {
    if (geometry().vertexCount() != 0) {
        geometry().allocate(0);
        markDirtyGeometry();
    }
}

void WaveformRenderLyrics::preprocess() {
    if (m_isSlipRenderer) {
        return;
    }

    // For allshader, skip if this is a slip renderer
    updateTexture();
}
} // namespace allshader
