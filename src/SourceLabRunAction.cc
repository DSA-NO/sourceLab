#include "SourceLabRunAction.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4UnitsTable.hh"

namespace SourceLab
{

SourceLabRunAction::SourceLabRunAction(SourceLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fRunDose);
  accumulableManager->Register(fRunEnergyDeposit);
  accumulableManager->Register(fRunTrackLength);

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->CreateH1("Dose", "Dose in sample", 100, 0., 10 * gray);
  analysisManager->CreateH1("Edep", "Energy deposit in sample", 100, 0., 10 * MeV);
  analysisManager->CreateH1("TrackL", "Track length in sample", 100, 0., 10 * cm);
  fSampleNtupleId = analysisManager->CreateNtuple("sample", "Sample dose, energy deposit, and track length");
  analysisManager->CreateNtupleDColumn("Dose");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->CreateNtupleDColumn("TrackL");
  analysisManager->FinishNtuple(fSampleNtupleId);

  fRunNtupleId = analysisManager->CreateNtuple("run", "Run-level sample summary");
  analysisManager->CreateNtupleIColumn("Events");
  analysisManager->CreateNtupleDColumn("Dose");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->CreateNtupleDColumn("TrackL");
  analysisManager->CreateNtupleIColumn("ThreadId");
  analysisManager->FinishNtuple(fRunNtupleId);
}

void SourceLabRunAction::BeginOfRunAction(const G4Run*)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Reset();
  analysisManager->OpenFile("sourceLab.root");

  if (isMaster) {
    G4cout << "Starting run." << G4endl;
    if (fDetectorConstruction) {
      G4cout << "World size: " << fDetectorConstruction->GetWorldSize() / m << " m" << G4endl;
      G4cout << "Sample depth: " << fDetectorConstruction->GetSampleDepth() / cm << " cm" << G4endl;
    }
  }
}

void SourceLabRunAction::EndOfRunAction(const G4Run* run)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Merge();

  auto* analysisManager = G4AnalysisManager::Instance();
  const auto writeRunRow = !G4Threading::IsMultithreadedApplication() || !isMaster;
  if (writeRunRow && fRunNtupleId >= 0) {
    analysisManager->FillNtupleIColumn(fRunNtupleId, 0, run->GetNumberOfEvent());
    analysisManager->FillNtupleDColumn(fRunNtupleId, 1, GetRunDose());
    analysisManager->FillNtupleDColumn(fRunNtupleId, 2, GetRunEnergyDeposit());
    analysisManager->FillNtupleDColumn(fRunNtupleId, 3, GetRunTrackLength());
    analysisManager->FillNtupleIColumn(fRunNtupleId, 4, G4Threading::G4GetThreadId());
    analysisManager->AddNtupleRow(fRunNtupleId);
  }
  analysisManager->Write();
  analysisManager->CloseFile(false);

  if (isMaster) {
    G4cout << "Run summary: " << run->GetNumberOfEvent() << " events" << G4endl;
    G4cout << "Total dose in sample: " << G4BestUnit(GetRunDose(), "Dose") << G4endl;
    G4cout << "Total energy deposit in sample: " << G4BestUnit(GetRunEnergyDeposit(), "Energy") << G4endl;
    G4cout << "Total charged track length in sample: " << G4BestUnit(GetRunTrackLength(), "Length") << G4endl;
  }
}

void SourceLabRunAction::AddEventScoring(G4double eventDose, G4double eventEnergyDeposit, G4double eventTrackLength)
{
  fRunDose += eventDose;
  fRunEnergyDeposit += eventEnergyDeposit;
  fRunTrackLength += eventTrackLength;
}

G4double SourceLabRunAction::GetRunDose() const
{
  return fRunDose.GetValue();
}

G4double SourceLabRunAction::GetRunEnergyDeposit() const
{
  return fRunEnergyDeposit.GetValue();
}

G4double SourceLabRunAction::GetRunTrackLength() const
{
  return fRunTrackLength.GetValue();
}

G4int SourceLabRunAction::GetSampleNtupleId() const
{
  return fSampleNtupleId;
}

}  // namespace SourceLab
