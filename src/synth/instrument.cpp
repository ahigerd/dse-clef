#include "instrument.h"
#include "dsecontext.h"
#include "utility.h"
#include "../chunks/wavichunk.h"
#include "../chunks/trackchunk.h"
#include <cmath>

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

Instrument::Instrument()
: context(nullptr), programId(-1), gain(1.0), pan(0.5)
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

Note Instrument::startNote(const TrkEvent& ev, int octave, int lastLength) const
{
  Note note(ev, octave, lastLength);
  for (const SplitInfo& split : splits) {
    if (note.pitch < split.lowKey || note.pitch > split.highKey ||
        note.velocity < split.lowVelocity || note.velocity > split.highVelocity) {
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
      note.useEnvelope = true;
      note.attackTime = table[split.attackTime] * mult;
      note.attackLevel = split.attackLevel / 127.0;
      note.holdTime = table[split.holdTime] * mult;
      note.decayTime = table[split.decayTime] * mult;
      note.sustainLevel = split.sustainLevel / 127.0;
      note.fadeTime = table[split.fadeTime] * mult;
      note.releaseTime = table[split.releaseTime] * mult;
      note.bendRange = split.bendRange;
    }
    note.sample = context ? context->findSample(split) : nullptr;
    if (note.sample) {
      // If using samples honor pan, tuning, and volume
      note.gain = gain * (split.volume / 127.0);
      //note.pitch -= split.transpose + (split.fineTune / 255.0);
    } else {
      // If using debug waves, honor pan and invert volume, assuming
      // it was used to equalize the volume of different samples.
      note.gain = gain * (2.0 - split.volume / 127.0);
    }
    note.pan = combinePan(pan, (split.pan / 128.0));
    break;
  }
  return note;
}
