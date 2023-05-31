#include "s2wcontext.h"
#include "commandargs.h"
#include "dsefile.h"
#include "riffwriter.h"
#include "actions/actions.h"

static std::map<std::string, ActionFn> actionFns{
  { "extract-dse", extractDSE },
  { "chunks", listChunks },
  { "dump-samples", dumpSamples },
  { "dump-streams", dumpStreams },
  { "synth", synthSequenceToFile },
};

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "output", "o", "dir", "Specify the path for saved files (default output/)" },
    { "extract-dse", "", "", "Scan input for DSE files and save to disk" },
    { "romaji", "", "", "(for --extract-dse) Attempt to convert Japanese filenames to romaji" },
    { "chunks", "", "", "List all chunks in the input files" },
    { "verbose", "v", "", "(for --chunks) Show more information about recognized chunks" },
    { "dump-samples", "", "", "Save all identified samples to disk" },
    { "dump-streams", "", "", "Save all identified streams to disk" },
    { "raw", "", "", "(for --dump-samples) Save raw samples without decoding or headers" },
    { "synth", "", "", "Render a smdl sequence to a wave file" },
    { "", "", "input", "Path(s) to the input file(s)" },
  });

  std::string argError = args.parse(argc, argv);
  if (!argError.empty()) {
    std::cerr << argError << std::endl;
    return 1;
  }

  bool actionError = true;
  std::string action;
  auto inputs = args.positional();
  if (args.hasKey("help")) {
    action = "help";
    actionError = false;
  } else {
    for (const auto& iter : actionFns) {
      if (args.hasKey(iter.first)) {
        if (action.empty()) {
          action = iter.first;
          actionError = inputs.size() == 0;
        } else {
          actionError = true;
          break;
        }
      }
    }
  }

  if (actionError || action == "help") {
    std::cout << args.usageText(argv[0]) << std::endl;
    std::cerr << std::endl;
    std::cerr << "Expected input files:" << std::endl;
    std::cerr << "  --extract-dse: One or more files of any format" << std::endl;
    std::cerr << "  --chunks: One or more files of any format produced by --extract-dse" << std::endl;
    std::cerr << "  --dump-samples: One or more .swd files" << std::endl;
    std::cerr << "  --dump-streams: One or more .sad files" << std::endl;
    std::cerr << "  --synth: One .smd file, and zero or more .swd files" << std::endl;
    std::cerr << "Only one mode may be specified." << std::endl;
#ifndef _WIN32
    std::cerr << std::endl;
    std::cerr << "When used with --synth, specifying - as an output path will send the rendered output" << std::endl;
    std::cerr << "to stdout, suitable for piping into another program for playback or transcoding." << std::endl;
#endif
    return actionError ? 1 : 0;
  }

  S2WContext s2w;
  std::string outputPath = args.getString("output", "./output");
  if (outputPath[outputPath.size() - 1] == '/') {
    outputPath.erase(outputPath.end() - 1, outputPath.end());
  }
  bool didSomething = actionFns[action](&s2w, inputs, outputPath, args);
  return didSomething ? 0 : 2;
}
