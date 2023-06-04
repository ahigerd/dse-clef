#include "instrument.h"
#include "track.h"
#include "sample.h"
#include "codec/sampledata.h"
#include "synth/sampler.h"
#include "synth/oscillator.h"
#include "s2wcontext.h"
#include "dsecontext.h"
#include "utility.h"
#include "../chunks/wavichunk.h"
#include "../chunks/trackchunk.h"
#define _USE_MATH_DEFINES
#include <cmath>

static const double expPitch = M_LN2 / 12.0;

static const double envDuration[2][128] = {
  {
       0,    1,    2,    3,    4,    5,    6,    7,
       8,    9,   10,   11,   12,   13,   14,   15,
      16,   17,   18,   19,   20,   21,   22,   23,
      24,   25,   26,   27,   28,   29,   30,   31,
      32,   35,   40,   45,   51,   57,   64,   72,
      80,   88,   98,  109,  120,  131,  144,  158,
     172,  188,  204,  222,  240,  260,  281,  303,
     327,  352,  378,  406,  435,  466,  498,  532,
     568,  606,  645,  686,  729,  775,  822,  871,
     923,  977, 1030, 1090, 1150, 1220, 1280, 1350,
    1420, 1570, 1650, 1740, 1820, 1910, 2010, 2100,
    2200, 2310, 2410, 2520, 2640, 2750, 2880, 3000,
    3130, 3260, 3400, 3550, 3690, 3840, 4000, 4160,
    4330, 4500, 4670, 4850, 5040, 5230, 5430, 5630,
    5840, 6050, 6270, 6490, 6720, 6960, 7200, 7450,
    7710, 7970, 8240, 8520, 8800, 9090, 1000, HUGE_VAL
  }, {
         0,      4,      7,     10,     15,     21,     28,     36,
        46,     58,     72,     87,    104,    123,    145,    168,
       389,    446,    508,    575,    648,    726,    810,    901,
       997,   1100,   1210,   1326,   1449,   1580,   1717,   1862,
      3023,   3264,   3517,   3782,   4060,   4351,   4655,   4972,
      5302,   5647,   6005,   6378,   6765,   7167,   7584,   8017,
     11286,  11904,  12544,  13205,  13889,  14594,  15323,  16074,
     16848,  17646,  18468,  19315,  20185,  21081,  22002,  22948,
     29900,  31147,  32428,  33742,  35089,  36471,  37887,  39338,
     40824,  42346,  43904,  45499,  47130,  48798,  50503,  52247,
     64834,  67019,  69250,  71528,  73854,  76228,  78651,  81122,
     83643,  86213,  88834,  91506,  94229,  97003,  99829, 102707,
    123245, 126727, 130272, 133879, 137551, 141286, 145086, 148951,
    152882, 156879, 160942, 165072, 169270, 173536, 177870, 182274,
    213424, 218616, 223888, 229241, 234676, 240193, 245793, 251476,
    257242, 263093, 269029, 275050, 281157, 287351, 293632, HUGE_VAL
  }
};

Instrument::Instrument(DSEContext* synth)
: context(synth), programId(-1), gain(1.0), pan(0.5)
{
  // initializers only
}

Instrument::Instrument(const ProgramInfo& preset, DSEContext* synth)
: context(synth), programId(preset.id), gain(preset.gain / 127.0), pan(preset.pan / 128.0),
  splits(preset.splits)
{
  for (const LFOInfo& lfo : preset.lfos) {
    if (LFO::isEnabled(lfo)) {
      lfos.emplace_back(lfo);
    }
  }
}

BaseNoteEvent* Instrument::makeEvent(Track* track, const TrkEvent& ev) const
{
  int eventDuration = ev.duration(track->lastNoteLength);
  track->lastNoteLength = eventDuration;
  if (!eventDuration) {
    return nullptr;
  }

  int noteNumber = ev.midiNote(track->octave);
  int bendRange = 2;
  const Sample* sample = nullptr;
  double pitch = noteNumber;
  int velocity = ev.velocity();
  double noteGain = 1.0;
  double notePan = pan;

  bool useEnvelope = false;
  double attackLevel = 1.0;
  double attackTime = 0;
  double holdTime = 0;
  double decayTime = 0;
  double sustainLevel = 0;
  double fadeTime = HUGE_VAL;
  double releaseTime = 0.08;

  for (const SplitInfo& split : splits) {
    if (noteNumber < split.lowKey || noteNumber > split.highKey ||
        velocity < split.lowVelocity || velocity > split.highVelocity) {
      continue;
    }
    if (split.envelope) {
      int tableIndex = 1;
      double mult = 1.0;
      if (split.envelopeMult) {
        tableIndex = 0;
        mult = (double)split.envelopeMult;
      }
      const double* table = envDuration[tableIndex];
      useEnvelope = true;
      attackTime = table[split.attackTime] * mult * .001;
      attackLevel = split.attackLevel / 127.0;
      holdTime = table[split.holdTime] * mult * .001;
      decayTime = table[split.decayTime] * mult * .001;
      sustainLevel = split.sustainLevel / 127.0;
      fadeTime = table[split.fadeTime] * mult * .001;
      releaseTime = table[split.releaseTime] * mult * .001;
      bendRange = split.bendRange;
    }
    sample = (context) ? context->findSample(split) : nullptr;
    if (sample) {
      // If using samples honor volume as-is
      noteGain = gain * (split.volume / 127.0);
      pitch -= sample->sampleInfo->rootKey;
    } else {
      // If using debug waves, invert volume, assuming it was used
      // to equalize the volume of different samples.
      noteGain = gain * (2.0 - split.volume / 127.0);
    }
    // TODO: does split pan replace instrument pan or combine with it?
    notePan = combinePan(notePan, (split.pan / 128.0));
    break;
  }

  if (track->detune) {
    pitch += (std::rand() / (0.5 * RAND_MAX) - 1.0) * track->detune;
  }

  BaseNoteEvent* event = nullptr;
  if (sample) {
    InstrumentNoteEvent* note = new InstrumentNoteEvent;
    note->pitch = pitch;
    note->intParams.push_back(sample->sample->sampleID);
    note->intParams.push_back(track->bendRange);
    note->intParams.push_back(bendRange);
    note->floatParams.push_back(track->pitchBend);
    event = note;
  } else {
    OscillatorEvent* osc = new OscillatorEvent;
    if (programId >= 0x7c) {
      osc->waveformID = 5;
      useEnvelope = true;
      attackTime = 0;
      attackLevel = 1.0;
      holdTime = 0;
      decayTime = 0.02;
      sustainLevel = 0.3;
      fadeTime = 0.5;
      releaseTime = 0.08;
      pitch = 120;
      velocity *= 1.5;
    } else {
      // TODO: PSG channels
      osc->waveformID = 0;
    }
    osc->frequency = TrkEvent::frequency(pitch, 0);
    event = osc;
  }

  noteGain = noteGain * (velocity / 127.0);
  event->timestamp = track->samplePos * context->sampleTime;
  event->duration = eventDuration * track->samplesPerTick * context->sampleTime;
  event->volume = noteGain * noteGain;
  event->pan = notePan;
  if (useEnvelope) {
    event->setEnvelope(
      attackLevel,
      attackTime,
      holdTime,
      decayTime,
      sustainLevel,
      fadeTime,
      releaseTime
    );
  }

  return event;
}

Channel::Note* Instrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  auto noteEvent = InstrumentNoteEvent::castShared(event);
  if (!noteEvent) {
    return DefaultInstrument::noteEvent(channel, event);
  }

  uint64_t sampleID = noteEvent->intParams[0];
  int bendRange = noteEvent->intParams[1];
  if (!bendRange) {
    bendRange = noteEvent->intParams[2];
  }
  double pitchBend = noteEvent->floatParams[0];
  double pitch = fastExp(noteEvent->pitch * expPitch);
  double duration = event->duration;

  SampleData* sampleData = channel->ctx->s2wContext()->getSample(sampleID);
  Sampler* samp = new Sampler(channel->ctx, sampleData, pitch);
  samp->param(AudioNode::Gain)->setConstant(noteEvent->volume);
  samp->param(AudioNode::Pan)->setConstant(noteEvent->pan);
  samp->param(Sampler::PitchBend)->setConstant(fastExp(pitchBend * bendRange * expPitch));
  if (!duration) {
    duration = sampleData->duration();
  }

  Channel::Note* note = new Channel::Note(event, samp, duration);

  DelayNode* filter = nullptr;
  for (int i = 0; i < lfos.size(); i++) {
    const LFO& lfo = lfos[i];
    std::shared_ptr<BaseOscillator> osc(makeLFO(lfo));
    if (!filter && (lfo.route == LFO::Volume || lfo.route == LFO::Pan)) {
      filter = new DelayNode(note->source);
      note->source.reset(filter);
    }
    if (lfo.route == LFO::Pitch) {
      samp->param(Sampler::PitchBend)->connect(osc, 1/8192.0, 1.0);
    } else if (lfo.route == LFO::Volume) {
      filter->addParam(AudioNode::Gain, 1.0);
      filter->param(AudioNode::Gain)->connect(osc, 1/(256.0 * 64.0), 1.0);
    } else if (lfo.route == LFO::Pan) {
      filter->addParam(AudioNode::Pan, 0.5);
      filter->param(AudioNode::Pan)->connect(osc, 1/1024.0, 0.5);
    }
  }

  if (event->useEnvelope) {
    applyEnvelope(channel, note);
  }
  return note;
}

BaseOscillator* Instrument::makeLFO(const LFO& lfo) const
{
  BaseOscillator* node;
  if (lfo.waveform == LFO::Square) {
    node = new SquareOscillator(context);
  } else if (lfo.waveform == LFO::Triangle) {
    node = new TriangleOscillator(context);
  } else if (lfo.waveform == LFO::Sine) {
    node = new SineOscillator(context);
  } else if (lfo.waveform == LFO::Saw) {
    node = new SawtoothOscillator(context);
  } else {
    node = new NoiseOscillator(context);
  }
  node->param(BaseOscillator::Frequency)->setConstant(lfo.frequencyHz);
  node->param(BaseOscillator::Gain)->setConstant(lfo.scale);
  return node;
}

void Instrument::modulatorEvent(Channel* channel, std::shared_ptr<ModulatorEvent> event)
{
  if ((event->param != Sampler::PitchBend && event->param != 'pbrg')) {
    DefaultInstrument::modulatorEvent(channel, event);
    return;
  }
  for (auto& pair : channel->notes) {
    auto note = InstrumentNoteEvent::castShared(pair.second->event);
    auto param = pair.second->source->param(Sampler::PitchBend);
    if (!note || !param) {
      continue;
    }
    if (event->param == 'pbrg') {
      note->intParams[1] = event->value;
    } else {
      note->floatParams[0] = event->value;
    }
    double bendRange = (note->intParams[1] == 0 ? note->intParams[2] : note->intParams[1]);
    double pitchBend = fastExp(note->floatParams[0] * bendRange * expPitch);
    param->setConstant(pitchBend);
  }
}
