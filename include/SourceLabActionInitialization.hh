#ifndef SourceLabActionInitialization_h
#define SourceLabActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

#include "globals.hh"

namespace SourceLab
{
class SourceLabDetectorConstruction;

class SourceLabActionInitialization : public G4VUserActionInitialization
{
  public:
    explicit SourceLabActionInitialization(SourceLabDetectorConstruction* detectorConstruction = nullptr,
      const G4String& emModel = "option4",
      G4bool enableRadioactiveDecay = false);
    ~SourceLabActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;

  private:
    SourceLabDetectorConstruction* fDetectorConstruction = nullptr;
    G4String fEmModel;
    G4bool fEnableRadioactiveDecay = false;
};

}  // namespace SourceLab

#endif
