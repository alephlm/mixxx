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
    if (not textColorStr.isEmpty()) {
        m_textColor = QColor(textColorStr);
    }
    const auto bgColorStr = context.selectString(node, "LyricsBackgroundColor");
    if (not bgColorStr.isEmpty()) {
        m_bgColor = QColor(bgColorStr);
    }
    const auto fontSize = context.selectString(node, "LyricsFontSize");
    if (not fontSize.isEmpty()) {
        m_font.setPointSize(fontSize.toInt());
    }
}

void WaveformRenderLyrics::onSetTrack() {
    m_cache.hasTexture = false;
}

void WaveformRenderLyrics::draw(QPainter* painter, QPaintEvent* event) {
    Q_UNUSED(painter);
    Q_UNUSED(event);
    DEBUG_ASSERT(false);
}

QString WaveformRenderLyrics::formatTime(double seconds) const {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void WaveformRenderLyrics::regenerateTexture() {
    const auto& track = m_waveformRenderer->getTrackInfo();
    if (not track) {
        clearGeometry();
        return;
    }

    const auto& lyrics = track->getLyrics();
    if (lyrics.isEmpty()) {
        clearGeometry();
        return;
    }

    int widgetWidth = m_waveformRenderer->getWidth();
    int widgetHeight = m_waveformRenderer->getHeight();
    float dpr = m_waveformRenderer->getDevicePixelRatio();
    if (widgetWidth <= 0 || widgetHeight <= 0) {
        clearGeometry();
        return;
    }

    double trackDuration = track->getDuration();
    if (trackDuration <= 0) {
        clearGeometry();
        return;
    }

    double firstPos = m_waveformRenderer->getFirstDisplayedPosition(
            ::WaveformRendererAbstract::Play);
    double lastPos = m_waveformRenderer->getLastDisplayedPosition(
            ::WaveformRendererAbstract::Play);
    double firstTime = firstPos * trackDuration;
    double lastTime = lastPos * trackDuration;
    double visibleDuration = lastTime - firstTime;
    if (visibleDuration <= 0) {
        clearGeometry();
        return;
    }

    int fontSize = qMax(8, widgetHeight / 22);
    QFont font("sans-serif", fontSize);
    QFontMetrics fm(font);

    int textH = fm.height();
    int labelH = textH + m_padding;
    int spacing = 4;
    int stripH = (labelH + spacing) * 2 + 6;

    // Small image: only the strip height, not full widget height
    int imgW = static_cast<int>(widgetWidth * dpr + 0.5f);
    int imgH = static_cast<int>(stripH * dpr + 0.5f);

    QImage image(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);

    {
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setFont(font);
        p.setClipRect(0, 0, widgetWidth, stripH);

        // Collect visible lines
        struct VL {
            float x;
            int w;
            QString text;
            int gi;
        };
        QVector<VL> vv;
        int gi = 0;
        for (const auto& line : lyrics) {
            if (line.text.isEmpty()) {
                gi++;
                continue;
            }
            double lt = line.timestampSeconds;
            if (lt < firstTime || lt > lastTime) {
                gi++;
                continue;
            }
            double ratio = (lt - firstTime) / visibleDuration;
            float x = static_cast<float>(ratio * widgetWidth);
            int tw = fm.horizontalAdvance(line.text);
            vv.append({x, tw + m_padding * 2, line.text, gi});
            gi++;
        }

        for (const auto& vl : vv) {
            // Short marker (only strip height)
            p.setPen(QColor(255, 255, 255, 40));
            p.drawLine(QPointF(vl.x, 0), QPointF(vl.x, stripH));

            float by = (vl.gi % 2) * (labelH + spacing) + 2;
            p.setBrush(m_bgColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(vl.x, by, vl.w, labelH, 3, 3);
            p.setPen(m_textColor);
            p.drawText(QPointF(vl.x + m_padding, by + fm.ascent() + m_padding / 2), vl.text);
        }
    }

    auto* ctx = m_waveformRenderer->getContext();
    dynamic_cast<TextureMaterial&>(material())
            .setTexture(std::make_unique<Texture>(ctx, image));

    geometry().allocate(6);
    TexturedVertexUpdater vu{geometry().vertexDataAs<Geometry::TexturedPoint2D>()};
    vu.addRectangle({0.f, 0.f},
            {static_cast<float>(widgetWidth), static_cast<float>(stripH)},
            {0.f, 0.f},
            {1.f, 1.f});

    markDirtyGeometry();
    markDirtyMaterial();

    m_cache.hasTexture = true;
}

void WaveformRenderLyrics::clearGeometry() {
    if (geometry().vertexCount() != 0) {
        geometry().allocate(0);
        markDirtyGeometry();
    }
    m_cache.hasTexture = false;
}

void WaveformRenderLyrics::preprocess() {
    if (m_isSlipRenderer)
        return;

    const auto& track = m_waveformRenderer->getTrackInfo();
    if (not track) {
        clearGeometry();
        return;
    }
    const auto& lyrics = track->getLyrics();
    if (lyrics.isEmpty()) {
        clearGeometry();
        return;
    }

    // Regenerate every frame to keep in sync with waveform updates
    regenerateTexture();
}
} // namespace allshader
