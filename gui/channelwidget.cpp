#include "channelwidget.h"
#include "vumeter.h"
#include "seq/sequenceevent.h"
#include "synth/synthcontext.h"
#include "synth/channel.h"
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>

ChannelWidget::ChannelWidget(QWidget* parent)
: QGroupBox(tr("Channels"), parent), context(nullptr), lastUpdate(0)
{
  new QGridLayout(this);
}

void ChannelWidget::contextUpdated(SynthContext* context)
{
  qDeleteAll(channels);
  this->context = context;
  lastUpdate = 0;
  if (!context) {
    return;
  }
  QGridLayout* grid = static_cast<QGridLayout*>(layout());
  int index = 0;
  for (const auto& channel : context->channels) {
    ChannelCheckBox* cb = new ChannelCheckBox(index, channel.get(), this);
    channels << cb;
    grid->addWidget(cb, index / 4, index % 4);
    index++;
  }
}

void ChannelWidget::updateMeters()
{
  double timestamp = context->currentTime();
  if (timestamp < lastUpdate + (1.0 / 120.0)) {
    return;
  }
  for (ChannelCheckBox* cb : channels) {
    cb->updateMeter(lastUpdate);
  }
  lastUpdate = timestamp;
}

ChannelCheckBox::ChannelCheckBox(int index, Channel* channel, ChannelWidget* parent)
: QWidget(parent), channel(channel)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  chk = new QCheckBox(QString::number(index + 1), this);
  chk->setFixedWidth(50);
  chk->setChecked(true);
  layout->addWidget(chk, 0);

  vu = new VUMeter(this);
  vu->setScaleMode(QAudio::LinearVolumeScale);
  vu->setChannels(1);
  layout->addWidget(vu, 1);

  QObject::connect(chk, SIGNAL(toggled(bool)), this, SLOT(setMute(bool)));
}

void ChannelCheckBox::setMute(bool checked)
{
  channel->mute = !checked;
}

void ChannelCheckBox::updateMeter(double timestamp)
{
  double level = channel->mute ? 0 : channel->notes.size() * channel->gain->valueAt(timestamp);
  vu->setLevel(0, level > 4.0 ? 1.0 : level / 4.0);
}
