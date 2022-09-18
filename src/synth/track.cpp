#include "track.h"
#include "dsecontext.h"
#include "instrument.h"
#include "sample.h"
#include "../dseutil.h"
#include "../chunks/trackchunk.h"
#include <iostream>

Track::Track(const TrackChunk* track, DSEContext* synth)
: trk(track), context(synth), currentInstrument(nullptr), eventPos(0), tickPos(0), samplePos(0),
  lastRestLength(0), lastNoteLength(0), octave(4), bendRange(0),
  volume(16.0), detune(0), pitchBend(0), expression(1.0),
  leftGain(1.0), rightGain(1.0)
{
  timingIter = context->allTimings[trk->trackID()].begin();
  timingEnd = context->allTimings[trk->trackID()].end();
}

bool Track::processEvent(std::vector<int16_t>& buffer)
{
  const TrkEvent& ev = trk->events[eventPos];
  bool mute = false; // (trk->trackID() != 6);// && (trk->trackID() != 6);

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

std::pair<double, double> Track::gain() const
{
  if (currentInstrument) {
    return std::make_pair(leftGain * currentInstrument->leftGain, rightGain * currentInstrument->rightGain);
  } else {
    return std::make_pair(leftGain, rightGain);
  }
}
