/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "phononwidget.h"

#include <KIconLoader>
#include <KLocalizedString>
#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/SeekSlider>
#include <Phonon/VideoWidget>

#include <QShowEvent>
#include <QToolButton>
#include <QVBoxLayout>

class EmbeddedVideoPlayer : public Phonon::VideoWidget
{
    Q_OBJECT

    public:
        EmbeddedVideoPlayer(QWidget *parent = nullptr) :
            Phonon::VideoWidget(parent)
        {
        }

        void setSizeHint(const QSize& size)
        {
            m_sizeHint = size;
            updateGeometry();
        }

        QSize sizeHint() const override
        {
            return m_sizeHint.isValid() ? m_sizeHint : Phonon::VideoWidget::sizeHint();
        }

    private:
        QSize m_sizeHint;
};

PhononWidget::PhononWidget(QWidget *parent)
    : QWidget(parent),
    m_url(),
    m_playButton(nullptr),
    m_stopButton(nullptr),
    m_topLayout(nullptr),
    m_media(nullptr),
    m_seekSlider(nullptr),
    m_audioOutput(nullptr),
    m_videoPlayer(nullptr)
{
}

void PhononWidget::setUrl(const QUrl &url)
{
    if (m_url != url) {
        stop(); // emits playingStopped() signal
        m_url = url;
    }
}

QUrl PhononWidget::url() const
{
    return m_url;
}

void PhononWidget::setVideoSize(const QSize& size)
{
    if (m_videoSize != size) {
        m_videoSize = size;
        applyVideoSize();
    }
}

QSize PhononWidget::videoSize() const
{
    return m_videoSize;
}

void PhononWidget::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) {
        QWidget::showEvent(event);
        return;
    }

    if (!m_topLayout) {
        m_topLayout = new QVBoxLayout(this);
        m_topLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(0);

        m_playButton = new QToolButton(this);
        m_stopButton = new QToolButton(this);
        m_seekSlider = new Phonon::SeekSlider(this);

        controlsLayout->addWidget(m_playButton);
        controlsLayout->addWidget(m_stopButton);
        controlsLayout->addWidget(m_seekSlider);

        m_topLayout->addLayout(controlsLayout);

        const int smallIconSize = IconSize(KIconLoader::Small);
        const QSize buttonSize(smallIconSize, smallIconSize);

        m_playButton->setToolTip(i18n("play"));
        m_playButton->setIconSize(buttonSize);
        m_playButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        m_playButton->setAutoRaise(true);
        connect(m_playButton, &QToolButton::clicked, this, &PhononWidget::play);

        m_stopButton->setToolTip(i18n("stop"));
        m_stopButton->setIconSize(buttonSize);
        m_stopButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
        m_stopButton->setAutoRaise(true);
        m_stopButton->hide();
        connect(m_stopButton, &QToolButton::clicked, this, &PhononWidget::stop);

        m_seekSlider->setIconVisible(false);

        // Creating an audio player or video player instance might take up to
        // 2 seconds when doing it the first time. To prevent that the user
        // interface gets noticeable blocked, the creation is delayed until
        // the play button has been pressed (see PhononWidget::play()).
    }
}

void PhononWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (!event->spontaneous()) {
        stop();
    }
}

void PhononWidget::stateChanged(Phonon::State newstate)
{
    setUpdatesEnabled(false);
    switch (newstate) {
    case Phonon::PlayingState:
    case Phonon::BufferingState:
        m_stopButton->show();
        m_playButton->hide();
        break;
    case Phonon::StoppedState:
        if (m_videoPlayer) {
            m_videoPlayer->hide();
        }
        emit hasVideoChanged(false);
        Q_FALLTHROUGH();
    default:
        m_stopButton->hide();
        m_playButton->show();
        break;
    }
    setUpdatesEnabled(true);
}

void PhononWidget::play()
{
    if (!m_media) {
        m_media = new Phonon::MediaObject(this);
        connect(m_media, &Phonon::MediaObject::stateChanged,
                this, &PhononWidget::stateChanged);
        connect(m_media, &Phonon::MediaObject::hasVideoChanged,
                this, &PhononWidget::slotHasVideoChanged);
        m_seekSlider->setMediaObject(m_media);
    }

    if (!m_videoPlayer) {
        m_videoPlayer = new EmbeddedVideoPlayer(this);
        m_topLayout->insertWidget(0, m_videoPlayer);
        Phonon::createPath(m_media, m_videoPlayer);
        applyVideoSize();
    }

    if (!m_audioOutput) {
        m_audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
        Phonon::createPath(m_media, m_audioOutput);
    }

    emit hasVideoChanged(false);

    m_media->setCurrentSource(m_url);
    m_media->hasVideo();
    m_media->play();
}

void PhononWidget::stop()
{
    if (m_media) {
        m_media->stop();
    }
}

void PhononWidget::slotHasVideoChanged(bool hasVideo)
{
    emit hasVideoChanged(hasVideo);

    if (hasVideo) {
        m_videoPlayer->show();
    }
}

void PhononWidget::applyVideoSize()
{
    if ((m_videoPlayer) && m_videoSize.isValid()) {
        m_videoPlayer->setSizeHint(m_videoSize);
    }
}

#include "phononwidget.moc"
