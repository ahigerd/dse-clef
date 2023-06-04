#ifndef SYNTH_INSTRUMENT_H
#define SYNTH_INSTRUMENT_H

#include "synth/iinstrument.h"
#include "seq/sequenceevent.h"
#include "../chunks/prgichunk.h"
#include "lfo.h"
class TrkEvent;
class DSEContext;
struct Track;
class BaseOscillator;

struct Instrument : public DefaultInstrument
{
  Instrument(DSEContext* synth);
  Instrument(const ProgramInfo& preset, DSEContext* synth);
  Instrument(const Instrument& other) = default;

  DSEContext* context;
  int programId;
  double gain, pan;

  std::vector<LFO> lfos;
  std::vector<SplitInfo> splits;

  BaseNoteEvent* makeEvent(Track* track, const TrkEvent& ev) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
  virtual void modulatorEvent(Channel* channel, std::shared_ptr<ModulatorEvent> event);

private:
  BaseOscillator* makeLFO(const LFO& lfo) const;
};

#endif
