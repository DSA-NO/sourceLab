#include "SourceLabRunAction.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

namespace SourceLab
{

SourceLabRunAction::SourceLabRunAction(SourceLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fRunEnergyDeposit);

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->CreateH1("Edep", "Energy deposit in sample", 100, 0., 10 * MeV);
  analysisManager->CreateNtuple("sample", "Sample energy deposit per event");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->FinishNtuple();

  analysisManager->CreateNtuple("run", "Run-level sample summary");
  analysisManager->CreateNtupleIColumn("Events");
  analysisManager->CreateNtupleDColumn("RunEdep");
  analysisManager->FinishNtuple(1);
}

void SourceLabRunAction::BeginOfRunAction(const G4Run*)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Reset();
  analysisManager->OpenFile("sourceLab.root");

  G4cout << "Starting run." << G4endl;
  if (fDetectorConstruction) {
    G4cout << "World size: " << fDetectorConstruction->GetWorldSize() / m << " m" << G4endl;
    G4cout << "Sample depth: " << fDetectorConstruction->GetSampleDepth() / cm << " cm" << G4endl;
  }
}

void SourceLabRunAction::EndOfRunAction(const G4Run* run)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Merge();

  auto* analysisManager = G4AnalysisManager::Instance();
  if (isMaster) {
    analysisManager->FillNtupleIColumn(1, 0, run->GetNumberOfEvent());
    analysisManager->FillNtupleDColumn(1, 1, GetRunEnergyDeposit());
    analysisManager->AddNtupleRow(1);
  }
  analysisManager->Write();
  analysisManager->CloseFile(false);

  G4cout << "Run summary: " << run->GetNumberOfEvent() << " events" << G4endl;
  G4cout << "Total energy deposit in sample: " << GetRunEnergyDeposit() / MeV << " MeV" << G4endl;
}

void SourceLabRunAction::AddEventEnergyDeposit(G4double eventEnergyDeposit)
{
  fRunEnergyDeposit += eventEnergyDeposit;
}

G4double SourceLabRunAction::GetRunEnergyDeposit() const
{
  return fRunEnergyDeposit.GetValue();
}

}  // namespace SourceLab
