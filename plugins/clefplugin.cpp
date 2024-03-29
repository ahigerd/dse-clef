#include "plugin/baseplugin.h"
#include "codec/sampledata.h"
#include "dsecontext.h"
#include "dsefile.h"
#include "dseutil.h"
#include "actions/actions.h"
#include <iostream>

#define ARM7_CLOCK 33513982
#define SAMPLE_RATE (ARM7_CLOCK / 1024.0)

#ifdef BUILD_CLAP
#include "plugin/clapplugin.h"
#include "seq/sequenceevent.h"
#include "synth/instrument.h"
#include "synth/sampler.h"

class DSEClapPlugin : public ClefClapPlugin<ClefPluginInfo>
{
public:
  DSEClapPlugin(const clap_host_t* host)
  : ClefClapPlugin<ClefPluginInfo>(host)
  {
    // initializers only
  }

protected:
  void prepareChannel(Channel* channel) {
    auto pitchBend = channel->param(Sampler::PitchBend);
    if (!pitchBend) {
      channel->addParam(Sampler::PitchBend, 0);
    }
  }

  BaseNoteEvent* createNoteEvent(const clap_event_note_t* event)
  {
    return static_cast<Instrument*>(currentInstrument())->makeEvent(event->key, event->velocity * 127);
  }
};
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
struct ClefPluginInfo : public TagsM3UMixin {
  CLEF_PLUGIN_STATIC_FIELDS

#ifdef BUILD_CLAP
  using ClapPlugin = DSEClapPlugin;
#endif

  static bool isPlayable(ClefContext*, const std::string&, std::istream& file) {
    // Implementations should check to see if the file is supported.
    // Return false or throw an exception to report failure.
    char magic[4];
    if (file.read(magic, 4)) {
      return (magic[0] == 's' && magic[1] == 'm' && magic[2] == 'd' && magic[3] == 'l');
    }
    return false;
  }

  static int sampleRate(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the sample rate of the file.
    // This can be hard-coded if the plugin always uses the same sample rate.
    return SAMPLE_RATE;
  }

  static double length(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the length of the file in seconds.
    auto buffer = readFile(file, filename);
    std::unique_ptr<DSEFile> dseFile(new DSEFile(ctx, buffer));
    DSEContext context(ctx, SAMPLE_RATE, std::move(dseFile), nullptr, nullptr);
    context.prepareTimings();
    return context.sampleLength / SAMPLE_RATE;
  }

  /*
  static TagMap readTags(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should read the tags from the file.
    // If the file format does not support embedded tags, consider
    // inheriting from TagsM3UMixin and removing this function.
    return TagMap();
  }
  */

  SynthContext* prepare(ClefContext* ctx, const std::string& filename, std::istream& file) {
    // Prepare to play the file. Load any necessary data into memory and store any
    // applicable state in members on this plugin object.

    // Be sure to call this to clear the sample cache:
    ctx->purgeSamples();

    TagMap tags = readTags(ctx, filename);
    std::string bank = tags["bank"];
    std::string pair = tags["pair"];
    DSEContext* synth = prepareSynthContext(ctx, file, filename, pair, bank, nullptr);
    return synth;
  }

  void release() {
    // Release any retained state allocated in prepare().
  }
};
#pragma GCC diagnostic pop

const std::string ClefPluginInfo::version = "0.0.3";
const std::string ClefPluginInfo::pluginName = "dse-clef";
const std::string ClefPluginInfo::pluginShortName = "dse-clef";
const std::string ClefPluginInfo::author = "Adam Higerd";
const std::string ClefPluginInfo::url = "https://bitbucket.org/ahigerd/dse-clef";
ConstPairList ClefPluginInfo::extensions = { { "smd", "SMD sequence files (*.smd)" } };
const std::string ClefPluginInfo::about =
  "dse-clef copyright (C) 2020-2023 Adam Higerd\n"
  "Distributed under the MIT license.";

#ifdef BUILD_CLAP
ConstPairList ClefPluginInfo::bankExtensions = { { "swd", "SWD bank files (*.swd)" } };
#endif

CLEF_PLUGIN(ClefPluginInfo);
