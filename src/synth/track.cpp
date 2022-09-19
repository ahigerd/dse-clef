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
: trk(track), context(synth)
{
  internalReset();
}

#if 0
bool Track::processEvent(std::vector<int16_t>& buffer)
{
  const TrkEvent& ev = trk->events[eventPos];
  bool mute = false; // (trk->trackID() != 6);

  if (ev.eventType == TrkEvent::SetOctave) {
    octave = ev.paramU8();
  } else if (ev.eventType == TrkEvent::AddOctave) {
    octave += ev.param8();
  } else if (ev.eventType == TrkEvent::SetVolume) {
    volume = ev.paramU8() / 127.0 * 16.0;
  } else if (ev.eventType == TrkEvent::SetExpression) {
    expression = ev.paramU8() / 127.0;
  } else if (ev.eventType == TrkEvent::SetDetuneRange) {
    detune = ev.param16() / 255.0;
  } else if (ev.eventType == TrkEvent::SetPitchBend) {
    // Interpretation? Assuming it matches default MIDI range
    // Notes say 500 = 1 semitone and negative means up
    pitchBend = ev.param16BE() / -4000.0;
    // Disassembly says:
    //   FUN_02073c90(ChanStructPtr, (int)(((uint)EventDataPtr[1] + (uint)*EventDataPtr * 0x100) * 0x10000) >> 0x10);
  } else if (ev.eventType == TrkEvent::SetPitchBendRange) {
    bendRange = ev.paramU8();
  } else if (ev.eventType == TrkEvent::SetPan) {
    rightGain = ev.paramU8() / 64.0;
    leftGain = 2.0 - rightGain;
  } else if (ev.eventType == TrkEvent::SetPreset) {
    currentInstrument = context->findInstrument(ev.paramU8());
  } else if (ev.isPlayNote() && !mute) {
    double startTime = samplePos * context->sampleTime;
    octave += ev.octaveOffset();
    Note note = currentInstrument ?
      currentInstrument->startNote(ev, startTime, octave, lastNoteLength) :
      Note(ev, startTime, octave, lastNoteLength);
    note.pitch += (std::rand() / (0.5 * RAND_MAX) - 1.0) * detune;
    lastNoteLength = note.remaining;

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
        /*
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
        */
      }
      samplePos += sampleLen;
      tickPos = nextTick;
    }
  }

  ++eventPos;
  return eventPos >= trk->events.size();
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
    case TrkEvent::SetChannelPan:
    case TrkEvent::SetChannelVolume:
    case TrkEvent::SetFineTune:
    case TrkEvent::AddToFineTune:
    case TrkEvent::SetCoarseTune:
    case TrkEvent::AddToTune:
    case TrkEvent::SweepTune:
    case TrkEvent::SetRandomNoteRange:
    case TrkEvent::SetDetuneRange:
      unhandled = true; break;
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
      volume = ev.paramU8() / 127.0 * 16.0;
      break;
    case TrkEvent::AddVolume:
      volume += ev.paramU8() / 127.0 * 16.0;
      break;
    case TrkEvent::SweepVolume:
    case TrkEvent::SetExpression:
      expression = ev.paramU8() / 127.0;
      break;
    case TrkEvent::ReplaceLFO2AsVolume:
    case TrkEvent::SetLFO2DelayFade:
    case TrkEvent::SetLFO2ToVolumeEnabled:
      unhandled = true; break;
    case TrkEvent::SetPan:
      pan = ev.paramU8() / 128.0;
      break;
    case TrkEvent::AddToPan:
      pan = clamp(pan + ev.paramU8() / 128.0, 0.0, 1.0);
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
        double startTime = samplePos * context->sampleTime;
        octave += ev.octaveOffset();
        Note note = currentInstrument ?
          currentInstrument->startNote(ev, 0, octave, lastNoteLength) :
          Note(ev, 0, octave, lastNoteLength);
        note.pitch += (std::rand() / (0.5 * RAND_MAX) - 1.0) * detune;
        lastNoteLength = note.remaining;
        if (lastNoteLength == 0) break;
        SequenceEvent* seqEvent;
        BaseNoteEvent* event;
        if (currentInstrument && note.sample) {
          SampleEvent* samp = new SampleEvent;
          samp->sampleID = note.sample->sample->sampleID;
          static const double log2inv = 1.0 / std::log(2);
          samp->pitchBend = std::pow(2.0, (note.pitch - note.sample->sampleInfo->rootKey + pitchBend) / 12.0);
          event = samp;
          seqEvent = samp;
        } else {
          OscillatorEvent* osc = new OscillatorEvent;
          if (currentInstrument && currentInstrument->programId >= 0x7c) {
            note.makeNoiseDrum();
            osc->waveformID = 5;
            note.pitch = 120;
            note.velocity *= 1.5;
          } else {
            osc->waveformID = 0;
          }
          osc->frequency = TrkEvent::frequency(note.pitch, pitchBend);
          event = osc;
          seqEvent = osc;
        }
        seqEvent->timestamp = startTime;
        event->duration = lastNoteLength * samplesPerTick * context->sampleTime;
        event->volume = (volume / 127.0) * (note.velocity / 127.0);
        event->pan = combinePan(pan, note.pan);
        event->setEnvelope(
            note.attackTime * context->sampleTime,
            note.holdTime * context->sampleTime,
            note.decayTime * context->sampleTime,
            note.sustainLevel,
            note.releaseTime * context->sampleTime);
        nextEvent = seqEvent;
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
  currentInstrument = nullptr;
  eventPos = 0;
  tickPos = 0;
  samplePos = 0;
  lastRestLength = 0;
  lastNoteLength = 0;
  octave = 4;
  bendRange = 0;
  volume = 16.0;
  detune = 0;
  pitchBend = 0;
  expression = 1.0;
  pan = 0.5;
  gain = 1.0;
  timingIter = context->allTimings[trk->trackID()].begin();
  timingEnd = context->allTimings[trk->trackID()].end();
  samplesPerTick = context->samplesPerTick(120);
}
