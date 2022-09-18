#include "dsecontext.h"
#include "dsefile.h"
#include "synth/instrument.h"
#include "synth/sample.h"
#include "chunks/trackchunk.h"
#include "chunks/songchunk.h"
#include "chunks/prgichunk.h"
#include "chunks/kgrpchunk.h"
#include "chunks/wavichunk.h"
#include "chunks/pcmdchunk.h"
#include <exception>

DSEContext::DSEContext(S2WContext* ctx, double sampleRate, std::unique_ptr<DSEFile> _smdl, std::unique_ptr<DSEFile> _swdl, DSEFile* bank)
: SynthContext(ctx, sampleRate, 2), //basePhasePerSample(1.0 / sampleRate),
  smdl(std::move(_smdl)), swdl(std::move(_swdl)), prgi(nullptr), bankPrgi(nullptr), wavi(nullptr), bankWavi(nullptr),
  kgrp(nullptr), bankKgrp(nullptr), pcmd(nullptr), bankPcmd(nullptr)
{
  if (bank) {
    bankWavi = bank->findChunk<WaviChunk>();
    bankPrgi = bank->findChunk<PrgiChunk>();
    bankKgrp = bank->findChunk<KgrpChunk>();
    bankPcmd = bank->findChunk<PcmdChunk>();
  }
  if (swdl) {
    wavi = swdl->findChunk<WaviChunk>();
    prgi = swdl->findChunk<PrgiChunk>();
    kgrp = swdl->findChunk<KgrpChunk>();
    pcmd = swdl->findChunk<PcmdChunk>();
    if (!wavi && !prgi && !kgrp && !pcmd) {
      // If the paired file exists but none of the chunks do, unload it
      swdl.reset(nullptr);
    }
  }
  // A hypothetical dsesf could put it all in the smdl
  if (!wavi) wavi = smdl->findChunk<WaviChunk>();
  if (!prgi) prgi = smdl->findChunk<PrgiChunk>();
  if (!kgrp) kgrp = smdl->findChunk<KgrpChunk>();
  if (!pcmd) pcmd = smdl->findChunk<PcmdChunk>();

  song = smdl->findChunk<SongChunk>();
  tracks = smdl->allChunks<TrackChunk>();
  if (!song || !tracks.size()) {
    throw std::runtime_error("No sequence data found");
  }

  ticksPerBeat = song->ticksPerBeat();
  tempos[0] = 120;
}

DSEContext::~DSEContext()
{
  // define dtor for forward-declared std::unique_ptr
}

void DSEContext::prepareTimings()
{
  int totalEvents = 0;
  int maxTick = 0;
  int loopTick = -1;
  for (const TrackChunk* trk : tracks) {
    int tickPos = 0;
    int lastRest = 0;
    int lastNote = 0;
    std::set<int> timings;
    for (const TrkEvent& ev : trk->events) {
      if (ev.isRest()) {
        // TODO: PollSilence
        lastRest = ev.duration(lastRest);
        tickPos += lastRest;
        timings.insert(tickPos);
      } else if (ev.isPlayNote()) {
        lastNote = ev.duration(lastNote);
        timings.insert(tickPos + lastNote);
      } else if (ev.eventType == TrkEvent::SetTempo || ev.eventType == TrkEvent::SetTempo2) {
        tempos[tickPos] = ev.paramU8();
      } else if (ev.eventType == TrkEvent::LoopPoint) {
        loopTick = tickPos;
      }
    }
    if (tickPos > maxTick) {
      maxTick = tickPos;
    }
    totalEvents += trk->events.size();
    allTimings[trk->trackID()] = timings;
  }
  int ticks = 0;
  int samplePos = 0;
  int tempo = tempos[0];
  double spt = samplesPerTick(tempo);
  loopSample = (loopTick < 0) ? -1 : 0;
  int lastTempoSample = 0;
  for (auto timeTempo : tempos) {
    for (auto& timings : allTimings) {
      timings.second.insert(timeTempo.first);
    }
    lastTempoSample = samplePos;
    int tickLen = timeTempo.first - ticks;
    if (loopTick >= 0 && timeTempo.first > loopTick) {
      // The loop point is before this tempo change
      loopSample = samplePos + (timeTempo.first - loopTick) * spt;
      loopTick = -1;
    }
    ticks = timeTempo.first;
    samplePos += tickLen * spt;
    tempo = timeTempo.second;
    spt = samplesPerTick(tempo);
  }
  if (loopTick >= 0) {
    // The loop point was after the last tempo change
    loopSample = lastTempoSample + (loopTick - tempos.rbegin()->first) * spt;
  }
  sampleLength = samplePos + (maxTick - ticks) * spt;
}

const Sample* DSEContext::findSample(const SplitInfo& split)
{
  int index = split.sampleIndex;
  auto iter = samples.find(index);
  if (iter != samples.end()) {
    return iter->second.get();
  }
  const SampleInfo* articulation = nullptr;
  const SampleInfo* data = nullptr;
  PcmdChunk* pcmSource = bankPcmd ? bankPcmd : pcmd;
  SampleData* pcm = nullptr;
  std::map<uint16_t, SampleInfo>::const_iterator infoIter;
  if (wavi && (infoIter = wavi->sampleInfo.find(index)) != wavi->sampleInfo.end()) {
    articulation = &infoIter->second;
  }
  if (bankWavi && (infoIter = bankWavi->sampleInfo.find(index)) != bankWavi->sampleInfo.end()) {
    data = &infoIter->second;
  }
  if (articulation && !data) {
    data = articulation;
  } else if (data && !articulation) {
    articulation = data;
  }
  if (data && pcmSource) {
    try {
      pcm = pcmSource->getSample(*data);
    } catch (...) {
      pcm = nullptr;
    }
  }
  Sample* sample = pcm ? new Sample(&split, articulation, pcm, this) : nullptr;
  samples.emplace(index, sample);
  return sample;
}

const Instrument* DSEContext::findInstrument(int presetId)
{
  auto iter = instruments.find(presetId);
  if (iter != instruments.end()) {
    return iter->second.get();
  }
  if (presetId < 0) {
    Instrument* debugWave = new Instrument;
    instruments.emplace(presetId, debugWave);
    return debugWave;
  }
  bool found = false;
  std::map<uint8_t, ProgramInfo>::const_iterator preset;
  if (prgi) {
    preset = prgi->programInfo.find(presetId);
    found = (preset != prgi->programInfo.end());
  }
  if (!found && bankPrgi) {
    preset = bankPrgi->programInfo.find(presetId);
    found = (preset != bankPrgi->programInfo.end());
  }
  if (!found) {
    return nullptr;
  }
  Instrument* inst = new Instrument(preset->second, this);
  instruments.emplace(presetId, inst);
  return inst;
}

