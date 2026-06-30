#pragma once

#include <QColor>
#include <QFont>

#include "rendergraph/geometrynode.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class QDomNode;
class SkinContext;
class ControlProxy;

namespace allshader {
class WaveformRenderLyrics;
} // namespace allshader

class allshader::WaveformRenderLyrics final
        : public QObject,
          public ::WaveformRendererAbstract,
          public rendergraph::GeometryNode {
    Q_OBJECT
  public:
    explicit WaveformRenderLyrics(
            WaveformWidgetRenderer* waveformWidget,
            ::WaveformRendererAbstract::PositionSource type =
                    ::WaveformRendererAbstract::Play);
    ~WaveformRenderLyrics() override;

    void draw(QPainter* painter, QPaintEvent* event) override final;
    void setup(const QDomNode& node, const SkinContext& skinContext) override;
    bool init() override;
    void onSetTrack() override;
    void preprocess() override;

  private:
    int findCurrentLineIndex(double currentTimeSeconds) const;
    QString formatTime(double seconds) const;
    void updateTexture();
    void clearGeometry();

    std::unique_ptr<ControlProxy> m_pPlayPosition;
    std::unique_ptr<ControlProxy> m_pPlay;

    QFont m_font;
    QColor m_textColor;
    QColor m_bgColor;
    int m_padding;
    bool m_isSlipRenderer;

    // Cached texture data
    struct {
        int width = 0;
        int height = 0;
        int bBoxWidth = 0;
        int bBoxHeight = 0;
        QString text;
        int lastLineIndex = -1;
    } m_cache;

    bool m_bTextureReady = false;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderLyrics);
};
