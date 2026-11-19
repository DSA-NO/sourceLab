#include "SourceLabEventAction.hh"

#include "SourceLabRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4SDManager.hh"
#include "G4THitsMap.hh"
#include "G4Threading.hh"
#include "G4VHitsCollection.hh"

#include <numeric>

namespace SourceLab
{
namespace
{
WaterScores GetRegionScores(const G4Event* event, G4int collectionId)
{
  auto* hce = event->GetHCofThisEvent();
  auto* hitsMap = collectionId >= 0 && hce && static_cast<std::size_t>(collectionId) < hce->GetCapacity()
    ? dynamic_cast<G4THitsMap<G4double>*>(hce->GetHC(collectionId)) : nullptr;
  if (!hitsMap) {
    G4ExceptionDescription message;
    message << "Missing or invalid phantom scoring collection " << collectionId << ".";
    G4Exception("SourceLabEventAction::GetRegionScores", "SourceLabScoring001", FatalException, message);
    return {};
  }

  WaterScores scores{};
  for (auto const& hit : *hitsMap->GetMap()) {
    if (hit.first < 0 || static_cast<std::size_t>(hit.first) >= scores.size() || !hit.second) {
      G4ExceptionDescription message;
      message << "Invalid phantom scoring copy number " << hit.first << ".";
      G4Exception("SourceLabEventAction::GetRegionScores", "SourceLabScoring002", FatalException, message);
      return {};
    }
    scores[hit.first] += *hit.second;
  }
  return scores;
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
  if (fPhantomDoseCollectionID < 0) {
    fPhantomDoseCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Phantom/Dose");
  }
  if (fPhantomEdepCollectionID < 0) {
    fPhantomEdepCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Phantom/Edep");
  }
  if (fPhantomTrackLengthCollectionID < 0) {
    fPhantomTrackLengthCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Phantom/TrackL");
  }

  const auto doses = GetRegionScores(anEvent, fPhantomDoseCollectionID);
  const auto energyDeposits = GetRegionScores(anEvent, fPhantomEdepCollectionID);
  const auto trackLengths = GetRegionScores(anEvent, fPhantomTrackLengthCollectionID);
  if (!fRunAction) {
    G4Exception("SourceLabEventAction::EndOfEventAction", "SourceLabScoring003", FatalException,
                "A run action is required for phantom scoring.");
    return;
  }

  const auto& regions = fRunAction->GetWaterRegions();
  G4double totalMass = 0.;
  for (const auto& region : regions) {
    totalMass += region.mass;
  }
  fEventEnergyDeposit = std::accumulate(energyDeposits.begin(), energyDeposits.end(), 0.);
  fEventTrackLength = std::accumulate(trackLengths.begin(), trackLengths.end(), 0.);
  fEventDose = fEventEnergyDeposit / totalMass;
  fRunAction->AddEventScoring(doses, energyDeposits, trackLengths);

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->FillH1(0, fEventDose);
  analysisManager->FillH1(1, fEventEnergyDeposit);
  analysisManager->FillH1(2, fEventTrackLength);
  const auto phantomNtupleId = fRunAction ? fRunAction->GetPhantomNtupleId() : -1;
  if (phantomNtupleId >= 0) {
    analysisManager->FillNtupleDColumn(phantomNtupleId, 0, fEventDose);
    analysisManager->FillNtupleDColumn(phantomNtupleId, 1, fEventEnergyDeposit);
    analysisManager->FillNtupleDColumn(phantomNtupleId, 2, fEventTrackLength);
    analysisManager->AddNtupleRow(phantomNtupleId);
  }

  const auto regionNtupleId = fRunAction->GetWaterRegionNtupleId();
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    analysisManager->FillNtupleIColumn(regionNtupleId, 0, anEvent->GetEventID());
    analysisManager->FillNtupleIColumn(regionNtupleId, 1, G4Threading::G4GetThreadId());
    analysisManager->FillNtupleIColumn(regionNtupleId, 2, static_cast<G4int>(i));
    analysisManager->FillNtupleDColumn(regionNtupleId, 3, doses[i]);
    analysisManager->FillNtupleDColumn(regionNtupleId, 4, energyDeposits[i]);
    analysisManager->FillNtupleDColumn(regionNtupleId, 5, trackLengths[i]);
    analysisManager->AddNtupleRow(regionNtupleId);
  }
}

}  // namespace SourceLab
