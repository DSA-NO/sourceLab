#include "SourceLabEventAction.hh"

#include "SourceLabRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4THitsMap.hh"
#include "G4VHitsCollection.hh"

namespace SourceLab
{

SourceLabEventAction::SourceLabEventAction(SourceLabRunAction* runAction)
: fRunAction(runAction)
{
}

void SourceLabEventAction::ResetEvent()
{
  fEventEnergyDeposit = 0.0;
}

G4double SourceLabEventAction::GetEventEnergyDeposit() const
{
  return fEventEnergyDeposit;
}

void SourceLabEventAction::BeginOfEventAction(const G4Event*)
{
  ResetEvent();
}

void SourceLabEventAction::EndOfEventAction(const G4Event* anEvent)
{
  if (fSampleEdepCollectionID < 0) {
    fSampleEdepCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("SampleSD/eDep");
    if (fSampleEdepCollectionID < 0) {
      return;
    }
  }

  auto* hce = anEvent->GetHCofThisEvent();
  if (!hce) {
    return;
  }

  if (fSampleEdepCollectionID >= hce->GetNumberOfCollections()) {
    return;
  }

  auto* hc = hce->GetHC(fSampleEdepCollectionID);
  if (!hc) {
    return;
  }

  auto* hitsMap = dynamic_cast<G4THitsMap<G4double>*>(hc);
  if (!hitsMap) {
    return;
  }

  auto* map = hitsMap->GetMap();
  for (auto const& hit : *map) {
    fEventEnergyDeposit += *(hit.second);
  }

  if (fRunAction) {
    fRunAction->AddEventEnergyDeposit(fEventEnergyDeposit);
  }

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillH1(0, fEventEnergyDeposit);
  analysisManager->FillNtupleDColumn(0, fEventEnergyDeposit);
  analysisManager->AddNtupleRow();
}

}  // namespace SourceLab
