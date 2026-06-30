#pragma once

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <algorithm>

namespace mixxx {
namespace audio {

/// A single line of lyrics with its associated timestamp in seconds.
struct LyricLine {
    double timestampSeconds = 0.0;
    QString text;

    bool operator<(const LyricLine& other) const {
        return timestampSeconds < other.timestampSeconds;
    }
};

using Lyrics = QList<LyricLine>;

/// Parse an LRC (LyRiCs) format string into a list of timestamped lines.
///
/// LRC format: [mm:ss.xx]text or [mm:ss.xxx]text
/// Also handles multiple timestamps: [00:12.00][00:17.20]Repeated line
/// And metadata tags: [ti:Title], [ar:Artist]
inline Lyrics parseLrc(const QString& lrcContent) {
    Lyrics lyrics;
    const QStringList lines = lrcContent.split('\n');
    // Regular expression for [mm:ss.xx] or [mm:ss.xxx] timestamps
    // Matches: [00:12.00], [00:12.000], [00:12.00]
    static const QRegularExpression tsRegex(
            R"(\[(\d{1,3}):(\d{2})(?:[\.:](\d{2,3}))?\])");

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        // Skip metadata tags like [ti:...], [ar:...], [al:...], [by:...], [offset:...]
        if (trimmed.startsWith('[') && trimmed.contains(':')) {
            // Check if it's a metadata tag (has text right after the colon, not a timestamp)
            int colonPos = trimmed.indexOf(':');
            if (colonPos > 1 && colonPos < 6) {
                // Check if this looks like a metadata tag (letters before colon)
                QString tag = trimmed.mid(1, colonPos - 1);
                static const QStringList metaTags = {"ti",
                        "ar",
                        "al",
                        "by",
                        "offset",
                        "re",
                        "tool",
                        "ve",
                        "length"};
                if (metaTags.contains(tag)) {
                    continue;
                }
            }
        }

        // Extract all timestamps from the line
        QList<double> timestamps;
        QRegularExpressionMatchIterator it = tsRegex.globalMatch(trimmed);
        QString textAfterTimestamps = trimmed;

        // Remove all matched timestamps from the text
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            double hundredths = 0.0;
            if (match.captured(3).length() == 3) {
                // milliseconds
                hundredths = match.captured(3).toDouble();
            } else if (!match.captured(3).isEmpty()) {
                // centiseconds
                hundredths = match.captured(3).toDouble() * 10.0;
            }
            double totalSeconds = minutes * 60.0 + seconds + hundredths / 1000.0;
            timestamps.append(totalSeconds);

            // Remove this timestamp from the text
            textAfterTimestamps.replace(match.captured(0), QString());
        }

        if (timestamps.isEmpty()) {
            // If no timestamp found, treat the whole line as untimed text
            // This can be useful for lyrics without timestamps
            continue;
        }

        textAfterTimestamps = textAfterTimestamps.trimmed();

        if (textAfterTimestamps.isEmpty()) {
            continue;
        }

        // Add a LyricLine for each timestamp
        for (double ts : timestamps) {
            lyrics.append({ts, textAfterTimestamps});
        }
    }

    // Sort by timestamp
    std::sort(lyrics.begin(), lyrics.end());

    return lyrics;
}

/// Load and parse an LRC file.
inline Lyrics loadLrcFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream stream(&file);
    // QTextStream defaults to UTF-8 in Qt6
    QString content = stream.readAll();
    file.close();
    return parseLrc(content);
}

/// Find an LRC file for a given audio file path.
/// Looks for a .lrc file with the same base name in the same directory.
inline QString findLrcFile(const QString& audioFilePath) {
    QFileInfo fileInfo(audioFilePath);
    QString lrcPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".lrc";
    if (QFile::exists(lrcPath)) {
        return lrcPath;
    }
    return {};
}

} // namespace audio
} // namespace mixxx
