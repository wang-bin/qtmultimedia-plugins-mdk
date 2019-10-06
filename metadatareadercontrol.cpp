/*
 * Copyright (C) 2018-2019 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "metadatareadercontrol.h"
#include "mediaplayercontrol.h"
#include "mdk/MediaInfo.h"
#include <QMediaMetaData>
#include <QSize>

MetaDataReaderControl::MetaDataReaderControl(MediaPlayerControl* mpc, QObject *parent)
    : QMetaDataReaderControl(parent)
    , mpc_(mpc)
{
    connect(mpc, &QMediaPlayerControl::durationChanged, this, &MetaDataReaderControl::readMetaData, Qt::DirectConnection);
}

void MetaDataReaderControl::readMetaData()
{
    using namespace QMediaMetaData;
    const auto& info = mpc_->player()->mediaInfo();
    QVariantMap m;
    m[Size] = (qint64)info.size;
    m[Duration] = (qint64)info.duration;
    m[QMediaMetaData::MediaType] = info.video.empty() ? "audio" : "video"; // FIXME: album cover image can be a video stream

    if (!info.metadata.empty()) { // TODO: metadata update
        struct {
            const char* key;
            QString qkey;
        } key_map[] = {
            {"title", Title},
            //{"Sub_Title", SubTitle},
            {"author", Author},
            {"comment", Comment},
            {"description", Description},//
            //{"category", Category}, // stringlist
            {"genre", Genre}, // stringlist
            {"year", Year}, // int
            {"date", Date}, // ISO 8601=>QDate
            //{"UserRating", UserRating},
            //{"Keywords", Keywords},
            {"language", Language},
            {"publisher", Publisher},
            {"copyright", Copyright},
            //{"ParentalRating", ParentalRating},
            //{"RatingOrganization", RatingOrganization},

            // movies
            {"performer", LeadPerformer},

            {"album", AlbumTitle},
            {"album_artist", AlbumArtist},
            //{"ContributingArtist", ContributingArtist},
            {"composer", Composer},
            //{"Conductor", Conductor},
            //{"Lyrics", Lyrics}, // AV_DISPOSITION_LYRICS
            //{"mood", Mood},
            {"track", TrackNumber},
            //{"CoverArtUrlSmall", CoverArtUrlSmall},
            //{"CoverArtUrlLarge", CoverArtUrlLarge},
            //{"AV_DISPOSITION_ATTACHED_PIC", CoverArtImage}, // qimage
            {"track", TrackNumber},
        };
        for (const auto& k : key_map) {
            const auto i = info.metadata.find(k.key);
            if (i != info.metadata.cend())
                m[k.qkey] = i->second.data();
        }
    }
// AV_DISPOSITION_TIMED_THUMBNAILS => ThumbnailImage. qimage
    if (!info.audio.empty()) {
        const auto& p = info.audio[0].codec;
        m[AudioBitRate] = (int)p.bit_rate;
        m[AudioCodec] = QString::fromStdString(p.codec);
        //m[AverageLevel]
        m[ChannelCount] = p.channels;
        m[SampleRate] = p.sample_rate;
    }

    if (!info.video.empty()) {
        const auto& p = info.video[0].codec;
        m[VideoFrameRate] = qreal(p.frame_rate);
        m[VideoBitRate] = (int)p.bit_rate;
        m[VideoCodec] = QString::fromStdString(p.codec);
        m[Resolution] = QSize(p.width, p.height);
        // PixelAspectRatio
    }

    bool avail_change = tag_.empty() != m.empty();
    tag_ = m;
    if (avail_change)
        Q_EMIT metaDataAvailableChanged(!tag_.isEmpty());
    Q_EMIT metaDataChanged();
}
