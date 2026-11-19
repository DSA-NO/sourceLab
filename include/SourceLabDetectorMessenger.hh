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

    // Geometry commands.
    G4UIcmdWithADoubleAndUnit* fSetWorldSizeCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetPhantomHeightCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetSourceRadiusCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fSetSourceShellThicknessCmd = nullptr;

    G4UIcommand* fUpdateCmd = nullptr;
    G4UIcommand* fPrintCmd = nullptr;
};

}  // namespace SourceLab

#endif
