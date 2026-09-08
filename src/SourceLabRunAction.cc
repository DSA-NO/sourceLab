#include "SourceLabRunAction.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Exception.hh"
#include "G4GenericMessenger.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4UnitsTable.hh"

#include <cctype>

namespace SourceLab
{

SourceLabRunAction::SourceLabRunAction(SourceLabDetectorConstruction* detectorConstruction,
  const G4String& emModel,
  G4bool enableRadioactiveDecay)
: fDetectorConstruction(detectorConstruction)
, fEmModel(emModel)
, fEnableRadioactiveDecay(enableRadioactiveDecay)
{
  ConfigureCommands();

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

  fRunInfoNtupleId = analysisManager->CreateNtuple("runinfo", "Run metadata for scenario provenance");
  analysisManager->CreateNtupleSColumn("Tag");
  analysisManager->CreateNtupleSColumn("Source");
  analysisManager->CreateNtupleSColumn("Geometry");
  analysisManager->CreateNtupleSColumn("Region");
  analysisManager->CreateNtupleDColumn("DepthCm");
  analysisManager->CreateNtupleSColumn("EMModel");
  analysisManager->CreateNtupleSColumn("RadioactiveDecay");
  analysisManager->CreateNtupleIColumn("Events");
  analysisManager->CreateNtupleIColumn("ThreadId");
  analysisManager->FinishNtuple(fRunInfoNtupleId);
}

void SourceLabRunAction::ConfigureCommands()
{
  fMessenger = std::make_unique<G4GenericMessenger>(this, "/sourceLab/output/", "Output metadata control");

  auto& tagCmd = fMessenger->DeclareMethod("tag", &SourceLabRunAction::SetOutputTag,
                                           "Set scenario tag used in output file naming.");
  tagCmd.SetGuidance("Set scenario tag used in output file naming.");
  tagCmd.SetGuidance("Allowed characters: letters, digits, '-' and '_'.");
  tagCmd.SetParameterName("tag", false);
  tagCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& sourceCmd = fMessenger->DeclareProperty("source", fOutputSource);
  sourceCmd.SetGuidance("Set source metadata string stored in runinfo ntuple.");
  sourceCmd.SetParameterName("source", false);
  sourceCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& geometryCmd = fMessenger->DeclareProperty("geometry", fOutputGeometry);
  geometryCmd.SetGuidance("Set geometry metadata string stored in runinfo ntuple.");
  geometryCmd.SetParameterName("geometry", false);
  geometryCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& regionCmd = fMessenger->DeclareProperty("region", fOutputRegion);
  regionCmd.SetGuidance("Set scoring-region metadata string stored in runinfo ntuple.");
  regionCmd.SetParameterName("region", false);
  regionCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& depthCmd = fMessenger->DeclareMethodWithUnit(
    "depth", "cm", &SourceLabRunAction::SetOutputDepth, "Set depth metadata stored in runinfo ntuple.");
  depthCmd.SetGuidance("Set depth metadata stored in runinfo ntuple.");
  depthCmd.SetParameterName("depth", false);
  depthCmd.SetRange("depth>=0.");
  depthCmd.SetStates(G4State_PreInit, G4State_Idle);
}

void SourceLabRunAction::SetOutputTag(const G4String& tag)
{
  if (tag.empty()) {
    G4ExceptionDescription msg;
    msg << "Invalid output tag '" << tag << "'. Tag must not be empty.";
    G4Exception("SourceLabRunAction::SetOutputTag", "SourceLabOutput001", FatalException, msg);
    return;
  }

  for (auto ch : tag) {
    const auto uch = static_cast<unsigned char>(ch);
    if (!std::isalnum(uch) && ch != '-' && ch != '_') {
      G4ExceptionDescription msg;
      msg << "Invalid output tag '" << tag
          << "'. Allowed characters are letters, digits, '-' and '_' only.";
      G4Exception("SourceLabRunAction::SetOutputTag", "SourceLabOutput002", FatalException, msg);
      return;
    }
  }

  fOutputTag = tag;
}

void SourceLabRunAction::SetOutputDepth(G4double depth)
{
  fOutputDepthCm = depth / cm;
}

G4String SourceLabRunAction::SanitizeForFileName(const G4String& value)
{
  G4String out;
  out.reserve(value.size());
  for (auto ch : value) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '-' || ch == '_') {
      out.push_back(ch);
    }
    else {
      out.push_back('_');
    }
  }
  return out;
}

G4String SourceLabRunAction::BuildOutputFileName() const
{
  const auto tag = SanitizeForFileName(fOutputTag);
  if (tag.empty()) {
    return "sourceLab-default.root";
  }
  return "sourceLab-" + tag + ".root";
}

void SourceLabRunAction::BeginOfRunAction(const G4Run*)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Reset();
  if (fOutputDepthCm < 0. && fDetectorConstruction) {
    fOutputDepthCm = fDetectorConstruction->GetSampleDepth() / cm;
  }
  analysisManager->OpenFile(BuildOutputFileName());

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
  if (writeRunRow && fRunInfoNtupleId >= 0) {
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 0, SanitizeForFileName(fOutputTag));
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 1, fOutputSource);
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 2, fOutputGeometry);
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 3, fOutputRegion);
    analysisManager->FillNtupleDColumn(fRunInfoNtupleId, 4, fOutputDepthCm);
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 5, fEmModel);
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 6, fEnableRadioactiveDecay ? "on" : "off");
    analysisManager->FillNtupleIColumn(fRunInfoNtupleId, 7, run->GetNumberOfEvent());
    analysisManager->FillNtupleIColumn(fRunInfoNtupleId, 8, G4Threading::G4GetThreadId());
    analysisManager->AddNtupleRow(fRunInfoNtupleId);
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
