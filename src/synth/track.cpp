#include "track.h"
#include "dsecontext.h"
#include "instrument.h"
#include "sample.h"
#include "../dseutil.h"
#include "../chunks/trackchunk.h"
#include "../chunks/wavichunk.h"
#include "codec/sampledata.h"
#include <iostream>
#include <cmath>

Track::Track(const TrackChunk* track, DSEContext* synth)
: trk(track), context(synth), channelID(track->channelID())
{
  internalReset();
}

#if 0
bool Track::processEvent(std::vector<int16_t>& buffer)
{
  const TrkEvent& ev = trk->events[eventPos];
  bool mute = false; // (trk->trackID() != 6);

  if (ev.isPlayNote() && !mute) {
    // TODO: add configuration option to use debug instruments
    /*
    if (currentInstrument && currentInstrument->programId >= 0x7c) {
      note.makeNoiseDrum();
    }
    */

    bool dupe = false;
    for (Note& other : notes) {
      if (other.noteNumber == note.noteNumber) {
        dupe = true;
        other = note;
        break;
      }
    }
    if (!dupe) {
      notes.push_back(note);
    }
  } else if (ev.isRest()) {
    lastRestLength = ev.duration(lastRestLength);
    int endTick = tickPos + lastRestLength;
    double baseMag = volume * expression;
    while (endTick > tickPos && timingIter != timingEnd) {
      ++timingIter;
      int nextTick = *timingIter;
      if (context->tempos.count(tickPos)) {
        samplesPerTick = context->samplesPerTick(context->tempos[tickPos]);
      }
      int tickLen = nextTick - tickPos;
      int sampleLen = tickLen * samplesPerTick;
      int startWord = samplePos << 1;
      int endWord = (samplePos + sampleLen) << 1;
      double startMs = samplePos * context->sampleTime;
      bool usePitchLFO = false, useVolumeLFO = false, usePanLFO = false;
      const std::vector<LFO>* lfos = nullptr;
      if (currentInstrument) {
        lfos = &currentInstrument->lfos;
        usePitchLFO = LFO::isEnabled(*lfos, LFO::Pitch);
        useVolumeLFO = LFO::isEnabled(*lfos, LFO::Volume);
        usePanLFO = LFO::isEnabled(*lfos, LFO::Pan);
      }
      for (int i = notes.size() - 1; i >= 0; --i) {
        Note& note = notes[i];
        double freq = TrkEvent::frequency(note.pitch, pitchBend * (bendRange ? bendRange : note.bendRange));
        double phasePerSample = context->basePhasePerSample * freq;
        double mag = baseMag * note.velocity;

        double ms = startMs;
        bool halt = false;
        for (int s = startWord; s < endWord; s += 2) {
          if (usePitchLFO) {
            freq = TrkEvent::frequency(note.pitch, LFO::applyAll(*lfos, LFO::Pitch, pitchBend, note.startTime, ms) * note.bendRange);
            phasePerSample = context->basePhasePerSample * freq;
          }
          double amp = note.envelope(ms);
          if (amp <= 0) {
            // envelope has faded to zero
            break;
          } else if (note.sample) {
            amp *= mag * note.sample->getSample(note.phase) * .0001;
          } else if (note.pitch < 0) {
            amp *= (std::rand() * 2.0 / RAND_MAX - 1) * mag * 4.0;
          } else {
            amp *= (note.phase < 0.5 ? mag : -mag);
          }
          if (useVolumeLFO) {
            amp *= LFO::applyAll(*lfos, LFO::Volume, 1, note.startTime, ms);
          }
          // TODO: apply instrument pan in realtime instead of on note-on
          if (usePanLFO) {
            double pan = clamp<double>(LFO::applyAll(*lfos, LFO::Pan, 0.5, note.startTime, ms), -1, 1);
            buffer[s] += amp * leftGain * note.leftGain * (1.0 - pan);
            buffer[s + 1] += amp * rightGain * note.rightGain * pan;
          } else {
            buffer[s] += amp * leftGain * note.leftGain;
            buffer[s + 1] += amp * rightGain * note.rightGain;
          }
          note.phase += phasePerSample;
          if (!note.sample && note.phase > 1) {
            note.phase -= 1;
          }
          ms += context->msPerSample;
        }

        note.remaining -= tickLen;
        if (ms > note.fadeTime) {
          notes.erase(notes.begin() + i);
        } else if (note.remaining <= 0) {
          note.release(ms);
        }
      }
    }
  }
}
#endif

bool Track::isFinished() const
{
  return eventPos >= trk->events.size();
}

double Track::length() const
{
  return double(context->sampleLength) / context->sampleRate;
}

std::shared_ptr<SequenceEvent> Track::readNextEvent()
{
  SequenceEvent* nextEvent = nullptr;
  if (channelEventPos < context->channelEvents[channelID].size()) {
    std::shared_ptr<ChannelEvent> event = context->channelEvents[channelID][channelEventPos];
    if (event->timestamp <= samplePos * context->sampleTime) {
      channelEventPos++;
      if (event->param == AudioNode::Gain) {
        channelVolume = event->value;
        nextEvent = updateTotalGain();
      } else if (event->param == AudioNode::Pan) {
        channelPan = event->value;
        nextEvent = updateTotalPan();
      }
    }
  }
  while (!nextEvent && !isFinished()) {
    const TrkEvent& ev = trk->events[eventPos];
    bool unhandled = false;

    switch (ev.eventType) {
    case TrkEvent::SetTempo:
    case TrkEvent::SetTempo2:
    case TrkEvent::EndOfTrack:
    case TrkEvent::LoopPoint:
      // handled in DSEContext::prepareTimings()
      break;
    case TrkEvent::SetSwdlAndBank:
    case TrkEvent::SetSwdl:
    case TrkEvent::SetBank:
      // handled in metadata for now
      break;
    case TrkEvent::PollSilence:
    case TrkEvent::Segno:
    case TrkEvent::DalSegno:
    case TrkEvent::ToCoda:
      unhandled = true; break;
    case TrkEvent::SetOctave:
      octave = ev.paramU8();
      break;
    case TrkEvent::AddOctave:
      octave += ev.paramU8();
      break;
    case TrkEvent::SetPreset:
      currentInstrument = context->findInstrument(ev.paramU8());
      if (!currentInstrument) {
        std::cerr << "trying to use unknown instrument " << int(ev.paramU8()) << std::endl;
      }
      if (currentInstrument && !context->getInstrument(ev.paramU8())) {
        context->registerInstrument(ev.paramU8(), std::unique_ptr<IInstrument>(new Instrument(*currentInstrument)));
      }
      nextEvent = new ChannelEvent('inst', uint64_t(ev.param8()));
      break;
    case TrkEvent::SweepSongVolume:
    case TrkEvent::DisableEnvelope:
    case TrkEvent::SetEnvelopeAttack:
    case TrkEvent::SetEnvelopeAttackTime:
    case TrkEvent::SetEnvelopeHold:
    case TrkEvent::SetEnvelopeDecaySustain:
    case TrkEvent::SetEnvelopeFade:
    case TrkEvent::SetEnvelopeRelease:
    case TrkEvent::SetNoteVolume:
      unhandled = true; break;
    case TrkEvent::SetChannelPan:
    case TrkEvent::SetChannelVolume:
      // Handled separately
      break;
    case TrkEvent::SetFineTune:
    case TrkEvent::AddToFineTune:
    case TrkEvent::SetCoarseTune:
    case TrkEvent::AddToTune:
    case TrkEvent::SweepTune:
    case TrkEvent::SetRandomNoteRange:
      unhandled = true; break;
    case TrkEvent::SetDetuneRange:
      detune = ev.param16() / 255.0;
      break;
    case TrkEvent::SetPitchBend:
      // Interpretation? Assuming it matches default MIDI range
      // Notes say 500 = 1 semitone and negative means up
      pitchBend = ev.param16BE() / -4000.0;
      // Disassembly says:
      //   FUN_02073c90(ChanStructPtr, (int)(((uint)EventDataPtr[1] + (uint)*EventDataPtr * 0x100) * 0x10000) >> 0x10);
      break;
    case TrkEvent::SetPitchBendRange:
      bendRange = ev.paramU8();
      break;
    case TrkEvent::ReplaceLFO1AsPitch:
    case TrkEvent::SetLFO1DelayFade:
    case TrkEvent::SetLFO1ToPitchEnabled:
      unhandled = true; break;
    case TrkEvent::SetVolume:
      volume = ev.paramU8() / 127.0;
      nextEvent = updateTotalGain();
      break;
    case TrkEvent::AddVolume:
      volume += ev.paramU8() / 127.0;
      nextEvent = updateTotalGain();
      break;
    case TrkEvent::SweepVolume:
      unhandled = true; break;
    case TrkEvent::SetExpression:
      expression = ev.paramU8() / 127.0;
      nextEvent = updateTotalGain();
      break;
    case TrkEvent::ReplaceLFO2AsVolume:
    case TrkEvent::SetLFO2DelayFade:
    case TrkEvent::SetLFO2ToVolumeEnabled:
      unhandled = true; break;
    case TrkEvent::SetPan:
      pan = ev.paramU8() / 128.0;
      nextEvent = updateTotalPan();
      break;
    case TrkEvent::AddToPan:
      pan = clamp(pan + ev.paramU8() / 128.0, 0.0, 1.0);
      nextEvent = updateTotalPan();
      break;
    case TrkEvent::SweepPan:
    case TrkEvent::ReplaceLFO3AsPan:
    case TrkEvent::SetLFO3DelayFade:
    case TrkEvent::SetLFO3ToPanEnabled:
    case TrkEvent::ReplaceLFO:
    case TrkEvent::SetLFODelayFade:
    case TrkEvent::SetLFOParam:
    case TrkEvent::SetLFORoute:
      unhandled = true; break;
    case TrkEvent::RepeatRest:
    case TrkEvent::AddRest:
    case TrkEvent::Rest8:
    case TrkEvent::Rest16:
    case TrkEvent::Rest24:
    default:
      if (ev.isPlayNote()) {
        octave += ev.octaveOffset();
        if (!currentInstrument) {
          std::cerr << "No inst" << std::endl;
          lastNoteLength = ev.duration(lastNoteLength);
        } else {
          nextEvent = currentInstrument->makeEvent(this, ev);
        }
      } else if (ev.isRest()) {
        lastRestLength = ev.duration(lastRestLength);
        int endTick = tickPos + lastRestLength;
        while (endTick > tickPos && timingIter != timingEnd) {
          ++timingIter;
          int nextTick = *timingIter;
          if (context->tempos.count(tickPos)) {
            samplesPerTick = context->samplesPerTick(context->tempos[tickPos]);
          }
          int tickLen = nextTick - tickPos;
          samplePos += tickLen * samplesPerTick;
          tickPos = nextTick;
        }
      } else {
        unhandled = true; break;
      }
    };

    if (unhandled) {
      std::cerr << "Unhandled event " << ev.debug(octave, lastNoteLength, lastRestLength) << std::endl;
    }
    eventPos++;
  }
  return std::shared_ptr<SequenceEvent>(nextEvent);
}

void Track::internalReset()
{
  currentInstrument = context->findInstrument(-1);
  eventPos = 0;
  tickPos = 0;
  samplePos = 0;
  lastRestLength = 0;
  lastNoteLength = 0;
  octave = 4;
  bendRange = 0;
  volume = 1.0;
  channelVolume = 1.0;
  detune = 0;
  pitchBend = 0;
  expression = 1.0;
  totalGain = 1.0;
  pan = 0.5;
  channelPan = 0.5;
  totalPan = 0.5;
  timingIter = context->allTimings[trk->trackID()].begin();
  timingEnd = context->allTimings[trk->trackID()].end();
  samplesPerTick = context->samplesPerTick(120);
  channelEventPos = 0;
}

SequenceEvent* Track::updateTotalGain()
{
  totalGain = volume * expression * channelVolume;
  return new ChannelEvent(AudioNode::Gain, totalGain);
}

SequenceEvent* Track::updateTotalPan()
{
  totalPan = combinePan(pan, channelPan);
  return new ChannelEvent(AudioNode::Pan, totalPan);
}
