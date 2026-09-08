#ifndef SourceLabEventAction_h
#define SourceLabEventAction_h 1

#include "G4SystemOfUnits.hh"
#include "G4UserEventAction.hh"
#include "globals.hh"

namespace SourceLab
{
class SourceLabRunAction;

class SourceLabEventAction : public G4UserEventAction
{
  public:
    explicit SourceLabEventAction(SourceLabRunAction* runAction = nullptr);
    ~SourceLabEventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

    void ResetEvent();
    G4double GetEventEnergyDeposit() const;

  private:
    SourceLabRunAction* fRunAction = nullptr;
    G4int fSampleEdepCollectionID = -1;
    G4double fEventEnergyDeposit = 0.0;
};

}  // namespace SourceLab

#endif
