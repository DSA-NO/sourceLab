#ifndef SourceLabDetectorMessenger_h
#define SourceLabDetectorMessenger_h 1

#include "G4UImessenger.hh"

class G4UIcmdWithADoubleAndUnit;
class G4UIcommand;

namespace SourceLab
{
class SourceLabDetectorConstruction;

class SourceLabDetectorMessenger : public G4UImessenger
{
  public:
    explicit SourceLabDetectorMessenger(SourceLabDetectorConstruction* detector);
    ~SourceLabDetectorMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String newValue) override;
    G4String GetCurrentValue(G4UIcommand* command) override;

  private:
    SourceLabDetectorConstruction* fDetector = nullptr;

    // Generic geometry commands.
    G4UIcmdWithADoubleAndUnit* fSetWorldSizeCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetPhantomHalfXCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetPhantomHalfYCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetPhantomHalfZCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetSampleDepthCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetSampleRadiusCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetSampleThicknessCmd = nullptr;

    G4UIcommand* fUpdateCmd = nullptr;
    G4UIcommand* fPrintCmd = nullptr;
};

}  // namespace SourceLab

#endif
