// Resolves macro execution working directories and normalizes macro paths.
// sourceLab - Geant4 dose calculation application

#ifndef SourceLabMacroRuntime_h
#define SourceLabMacroRuntime_h 1

#include "globals.hh"

namespace SourceLab
{
namespace MacroRuntime
{

struct RuntimeMacroConfig
{
  G4String batchMacroArg;
  G4String visMacroArg;
  G4String workingDirectory;
};

RuntimeMacroConfig ResolveRuntimeMacroConfig(char** argv,
                                             const G4String& macro,
                                             const G4String& visMacro);

G4bool ApplyWorkingDirectory(const RuntimeMacroConfig& config, G4String& warning);

}  // namespace MacroRuntime
}  // namespace SourceLab

#endif