/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * AI participated
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkplayercontrol.h"

#include <QtMultimedia/qmediaplayer.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QBuffer>
#include <QDataStream>
#include <QTemporaryFile>

namespace {

// Build a tiny valid PCM file so the loading-then-clearing test exercises
// MDK's asynchronous prepare path.
QByteArray shortWav()
{
    constexpr quint16 channels = 1;
    constexpr quint32 sampleRate = 8000;
    constexpr quint16 bitsPerSample = 16;
    constexpr quint32 sampleCount = sampleRate / 10;
    constexpr quint32 dataSize = sampleCount * channels * bitsPerSample / 8;

    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataSize);
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << channels << sampleRate;
    out << quint32(sampleRate * channels * bitsPerSample / 8);
    out << quint16(channels * bitsPerSample / 8) << bitsPerSample;
    out.writeRawData("data", 4);
    out << dataSize;
    result.append(QByteArray(int(dataSize), '\0'));
    return result;
}

} // namespace

class tst_MDKPlayerControl : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySourceResetsState();
    void emptySourceWhileLoadingIgnoresOldCallbacks();
    void emptyUrlWithStreamIsStillAValidSource();
};

void tst_MDKPlayerControl::emptySourceResetsState()
{
    QMediaPlayer owner;
    MDKPlayerControl control(&owner);

    control.setMedia({}, nullptr);

    QCOMPARE(control.media(), QUrl{});
    QCOMPARE(control.mediaStream(), nullptr);
    QCOMPARE(control.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(control.state(), QMediaPlayer::StoppedState);
    QCOMPARE(control.position(), 0);
    QCOMPARE(control.duration(), 0);
    QCOMPARE(control.bufferProgress(), 0.f);
    QVERIFY(!control.isSeekable());
    QVERIFY(!control.isAudioAvailable());
    QVERIFY(!control.isVideoAvailable());
    QVERIFY(control.metaData().isEmpty());
    QCOMPARE(control.trackCount(QPlatformMediaPlayer::VideoStream), 0);
    QCOMPARE(control.trackCount(QPlatformMediaPlayer::AudioStream), 0);
    QCOMPARE(control.trackCount(QPlatformMediaPlayer::SubtitleStream), 0);
    QCOMPARE(control.activeTrack(QPlatformMediaPlayer::VideoStream), -1);
    QCOMPARE(control.activeTrack(QPlatformMediaPlayer::AudioStream), -1);
    QCOMPARE(control.activeTrack(QPlatformMediaPlayer::SubtitleStream), -1);
}

void tst_MDKPlayerControl::emptySourceWhileLoadingIgnoresOldCallbacks()
{
    QTemporaryFile mediaFile;
    QVERIFY(mediaFile.open());
    const QByteArray media = shortWav();
    QCOMPARE(mediaFile.write(media), media.size());
    mediaFile.close();

    QMediaPlayer owner;
    MDKPlayerControl control(&owner);
    QSignalSpy statusSpy(&owner, &QMediaPlayer::mediaStatusChanged);
    QSignalSpy errorSpy(&owner, &QMediaPlayer::errorOccurred);

    control.setMedia(QUrl::fromLocalFile(mediaFile.fileName()), nullptr);
    control.setMedia({}, nullptr);
    statusSpy.clear();
    errorSpy.clear();

    QTest::qWait(250);

    QCOMPARE(control.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(control.state(), QMediaPlayer::StoppedState);
    QCOMPARE(control.duration(), 0);
    QCOMPARE(errorSpy.count(), 0);
    for (const auto &arguments : statusSpy)
        QCOMPARE(arguments.at(0).value<QMediaPlayer::MediaStatus>(), QMediaPlayer::NoMedia);
}

void tst_MDKPlayerControl::emptyUrlWithStreamIsStillAValidSource()
{
    QBuffer stream;
    stream.setData(shortWav());
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QMediaPlayer owner;
    MDKPlayerControl control(&owner);

    control.setMedia({}, &stream);

    QCOMPARE(control.media(), QUrl{});
    QCOMPARE(control.mediaStream(), &stream);
    QCOMPARE(QByteArray(control.player()->url()), QByteArray("stream:"));
}

QTEST_GUILESS_MAIN(tst_MDKPlayerControl)

#include "tst_mdkplayercontrol.moc"
