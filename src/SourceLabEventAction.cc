#include "SourceLabEventAction.hh"

#include "SourceLabRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4THitsMap.hh"
#include "G4VHitsCollection.hh"

namespace SourceLab
{
namespace
{
G4double GetHitsMapSum(const G4Event* event, G4int collectionId)
{
  if (collectionId < 0) {
    return 0.0;
  }

  auto* hce = event->GetHCofThisEvent();
  if (!hce || collectionId >= hce->GetNumberOfCollections()) {
    return 0.0;
  }

  auto* hc = hce->GetHC(collectionId);
  if (!hc) {
    return 0.0;
  }

  auto* hitsMap = dynamic_cast<G4THitsMap<G4double>*>(hc);
  if (!hitsMap) {
    return 0.0;
  }

  G4double sum = 0.0;
  for (auto const& hit : *hitsMap->GetMap()) {
    sum += *(hit.second);
  }
  return sum;
}
}

SourceLabEventAction::SourceLabEventAction(SourceLabRunAction* runAction)
: fRunAction(runAction)
{
}

void SourceLabEventAction::ResetEvent()
{
  fEventDose = 0.0;
  fEventEnergyDeposit = 0.0;
  fEventTrackLength = 0.0;
}

G4double SourceLabEventAction::GetEventDose() const
{
  return fEventDose;
}

G4double SourceLabEventAction::GetEventEnergyDeposit() const
{
  return fEventEnergyDeposit;
}

G4double SourceLabEventAction::GetEventTrackLength() const
{
  return fEventTrackLength;
}

void SourceLabEventAction::BeginOfEventAction(const G4Event*)
{
  ResetEvent();
}

void SourceLabEventAction::EndOfEventAction(const G4Event* anEvent)
{
  if (fSampleDoseCollectionID < 0) {
    fSampleDoseCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Sample/Dose");
  }
  if (fSampleEdepCollectionID < 0) {
    fSampleEdepCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Sample/Edep");
  }
  if (fSampleTrackLengthCollectionID < 0) {
    fSampleTrackLengthCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Sample/TrackL");
  }

  fEventDose = GetHitsMapSum(anEvent, fSampleDoseCollectionID);
  fEventEnergyDeposit = GetHitsMapSum(anEvent, fSampleEdepCollectionID);
  fEventTrackLength = GetHitsMapSum(anEvent, fSampleTrackLengthCollectionID);

  if (fRunAction) {
    fRunAction->AddEventScoring(fEventDose, fEventEnergyDeposit, fEventTrackLength);
  }

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillH1(0, fEventDose);
  analysisManager->FillH1(1, fEventEnergyDeposit);
  analysisManager->FillH1(2, fEventTrackLength);
  const auto sampleNtupleId = fRunAction ? fRunAction->GetSampleNtupleId() : -1;
  if (sampleNtupleId >= 0) {
    analysisManager->FillNtupleDColumn(sampleNtupleId, 0, fEventDose);
    analysisManager->FillNtupleDColumn(sampleNtupleId, 1, fEventEnergyDeposit);
    analysisManager->FillNtupleDColumn(sampleNtupleId, 2, fEventTrackLength);
    analysisManager->AddNtupleRow(sampleNtupleId);
  }
}

}  // namespace SourceLab
