#ifndef D2W_MERGETRACK_H
#define D2W_MERGETRACK_H

#include "seq/itrack.h"
#include <vector>
#include <utility>
#include <set>
class DSEContext;
class Instrument;

class MergeTrack : public ITrack
{
public:
  MergeTrack();

  void addTrack(const std::shared_ptr<ITrack>& track);

  virtual bool isFinished() const;
  virtual double length() const;

protected:
  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  virtual void internalReset();

private:
  std::vector<std::shared_ptr<ITrack>> subTracks;
  std::vector<std::shared_ptr<SequenceEvent>> nextEvents;
};

#endif
