#ifndef SourceLabRunAction_h
#define SourceLabRunAction_h 1

#include "G4Accumulable.hh"
#include "G4UserRunAction.hh"
#include "globals.hh"

namespace SourceLab
{
class SourceLabDetectorConstruction;

class SourceLabRunAction : public G4UserRunAction
{
  public:
    explicit SourceLabRunAction(SourceLabDetectorConstruction* detectorConstruction = nullptr);
    ~SourceLabRunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
    void AddEventEnergyDeposit(G4double eventEnergyDeposit);
    G4double GetRunEnergyDeposit() const;
    G4int GetSampleNtupleId() const;

  private:
    SourceLabDetectorConstruction* fDetectorConstruction = nullptr;
    G4Accumulable<G4double> fRunEnergyDeposit = 0.0;
    G4int fSampleNtupleId = -1;
    G4int fRunNtupleId = -1;
};

}  // namespace SourceLab

#endif
