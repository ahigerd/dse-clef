#include "actions.h"
#include "clefcontext.h"
#include "../dsefile.h"
#include "../dseutil.h"
#include <iostream>
#include "../chunks/trackchunk.h"
#include "../chunks/prgichunk.h"

bool listChunks(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  bool verbose = args.hasKey("verbose");
  for (const std::string& filename : paths) {
    DSEFile dseFile(ctx, filename);
    std::cout << filename << std::endl;
    std::cout << "\t" << dseFile.originalFilename() << ": " << magicString(dseFile.magic()) << std::endl;
    int chunkCount = dseFile.chunkCount();
    for (int i = 0; i < chunkCount; i++) {
      DSEChunk* chunk = dseFile.chunk(i);
      std::cout << "\t\tChunk: " << magicString(chunk->magic()) << " (" << chunk->size() << " bytes)" << std::endl;
      if (verbose) {
        std::cout << chunk->debug("\t\t\t") << std::flush;
      }
    }
    std::cout << std::endl;
  }
  return paths.size() > 0;
}
