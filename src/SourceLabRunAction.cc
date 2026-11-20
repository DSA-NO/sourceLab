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
#include "G4UImanager.hh"
#include "G4UnitsTable.hh"
#include "G4Version.hh"
#include "Randomize.hh"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace SourceLab
{

namespace
{
G4String ToLower(const G4String& value)
{
  G4String out = value;
  for (auto& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

bool TryCanonicalSource(const G4String& source, G4String& canonical)
{
  struct Alias
  {
    const char* label;
    const char* canonical;
  };

  static constexpr std::array<Alias, 9> kAliases{{
    {"unspecified", "unspecified"},
    {"co60", "co60"},
    {"cobalt-60", "co60"},
    {"cs137", "cs137"},
    {"cs-137", "cs137"},
    {"6mv", "6mv"},
    {"linac-6mv", "6mv"},
    {"10mv", "10mv"},
    {"linac-10mv", "10mv"},
  }};

  const auto normalized = ToLower(source);
  for (const auto& entry : kAliases) {
    if (normalized == entry.label) {
      canonical = entry.canonical;
      return true;
    }
  }
  return false;
}

constexpr const char* kSourceCandidates = "unspecified co60 cs137 6mv 10mv";

// Scientific notation avoids G4BestUnit rounding tiny-but-real values down to a displayed "0".
G4String ToScientific(G4double value, G4double unit, const G4String& unitLabel)
{
  std::ostringstream oss;
  oss << std::scientific << std::setprecision(6) << (value / unit) << " " << unitLabel;
  return oss.str();
}
}  // namespace

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
  accumulableManager->Register(&fScoreStatistics);
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    accumulableManager->Register(fRegionDoses[i]);
    accumulableManager->Register(fRegionEnergyDeposits[i]);
    accumulableManager->Register(fRegionTrackLengths[i]);
    accumulableManager->Register(&fRegionStatistics[i]);
  }

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->CreateH1("Dose", "Dose in water phantom", 100, 0., 10 * gray);
  analysisManager->CreateH1("Edep", "Energy deposit in water phantom", 100, 0., 10 * MeV);
  analysisManager->CreateH1("TrackL", "Track length in water phantom", 100, 0., 10 * cm);
  fPhantomNtupleId = analysisManager->CreateNtuple("phantom", "Water phantom dose, energy deposit, and track length");
  analysisManager->CreateNtupleDColumn("Dose");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->CreateNtupleDColumn("TrackL");
  analysisManager->FinishNtuple(fPhantomNtupleId);

  fRunNtupleId = analysisManager->CreateNtuple("run", "Run-level phantom summary");
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
  analysisManager->CreateNtupleSColumn("Geant4Version");
  analysisManager->CreateNtupleSColumn("RandomEngine");
  analysisManager->CreateNtupleSColumn("RandomState");
  analysisManager->FinishNtuple(fRunInfoNtupleId);

  fWaterRegionNtupleId = analysisManager->CreateNtuple("water_regions", "Per-event water region scores");
  analysisManager->CreateNtupleIColumn("EventId");
  analysisManager->CreateNtupleIColumn("ThreadId");
  analysisManager->CreateNtupleIColumn("Region");
  analysisManager->CreateNtupleDColumn("Dose");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->CreateNtupleDColumn("TrackL");
  analysisManager->FinishNtuple(fWaterRegionNtupleId);

  fWaterRegionRunNtupleId = analysisManager->CreateNtuple("water_region_run", "Run-level water region scores");
  analysisManager->CreateNtupleIColumn("ThreadId");
  analysisManager->CreateNtupleIColumn("Region");
  analysisManager->CreateNtupleDColumn("InnerRadiusMm");
  analysisManager->CreateNtupleDColumn("OuterRadiusMm");
  analysisManager->CreateNtupleDColumn("MassG");
  analysisManager->CreateNtupleDColumn("Dose");
  analysisManager->CreateNtupleDColumn("Edep");
  analysisManager->CreateNtupleDColumn("TrackL");
  analysisManager->FinishNtuple(fWaterRegionRunNtupleId);
  fStatisticsNtupleId = SourceLabScoreStatistics::CreateNtuple();
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

  auto& sourceCmd = fMessenger->DeclareMethod("source", &SourceLabRunAction::SetOutputSource,
                                               "Set source metadata stored in runinfo ntuple.");
  sourceCmd.SetGuidance("Set source metadata stored in runinfo ntuple.");
  sourceCmd.SetGuidance("Allowed values: unspecified, co60, cs137, 6mv, 10mv.");
  sourceCmd.SetParameterName("source", false);
  sourceCmd.SetCandidates(kSourceCandidates);
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

void SourceLabRunAction::SetOutputSource(const G4String& source)
{
  G4String canonical;
  if (!TryCanonicalSource(source, canonical)) {
    G4ExceptionDescription msg;
    msg << "Invalid source metadata '" << source
        << "'. Allowed values: unspecified, co60, cs137, 6mv, 10mv.";
    G4Exception("SourceLabRunAction::SetOutputSource", "SourceLabOutput003", FatalException, msg);
    return;
  }

  fOutputSource = canonical;
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
  analysisManager->OpenFile(BuildOutputFileName());

  if (fEnableRadioactiveDecay
      && (!G4Threading::IsMultithreadedApplication() || !isMaster)) {
    auto* uiManager = G4UImanager::GetUIpointer();
    const auto status = uiManager->ApplyCommand(
      "/process/had/rdm/thresholdForVeryLongDecayTime 1e+60 year");
    if (status != 0) {
      G4ExceptionDescription msg;
      msg << "Failed to set global long-decay-time threshold; UI status=" << status;
      G4Exception("SourceLabRunAction::BeginOfRunAction", "SourceLabDecay001", FatalException, msg);
      return;
    }
  }

  if (isMaster) {
    G4cout << "Starting run." << G4endl;
    if (fDetectorConstruction) {
      G4cout << "World size: " << fDetectorConstruction->GetWorldSize() / m << " m" << G4endl;
    }
  }
}

void SourceLabRunAction::EndOfRunAction(const G4Run* run)
{
  auto* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Merge();

  auto* analysisManager = G4AnalysisManager::Instance();
  const auto writeRunRow = !G4Threading::IsMultithreadedApplication() || !isMaster;
  const auto finalizeOutput = !G4Threading::IsMultithreadedApplication() || isMaster;
  const auto& regions = GetWaterRegions();
  G4double totalMass = 0.;
  G4double weightedDose = 0.;
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    totalMass += regions[i].mass;
    weightedDose += fRegionDoses[i].GetValue() * regions[i].mass;
    if (writeRunRow) {
      analysisManager->FillNtupleIColumn(fWaterRegionRunNtupleId, 0, G4Threading::G4GetThreadId());
      analysisManager->FillNtupleIColumn(fWaterRegionRunNtupleId, 1, static_cast<G4int>(i));
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 2, regions[i].innerRadius / mm);
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 3, regions[i].outerRadius / mm);
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 4, regions[i].mass / g);
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 5, fRegionDoses[i].GetValue());
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 6, fRegionEnergyDeposits[i].GetValue());
      analysisManager->FillNtupleDColumn(fWaterRegionRunNtupleId, 7, fRegionTrackLengths[i].GetValue());
      analysisManager->AddNtupleRow(fWaterRegionRunNtupleId);
    }
  }
  weightedDose /= totalMass;
  const auto doseFromEnergy = GetRunEnergyDeposit() / totalMass;
  const auto runDose = GetRunDose();
  const auto scale = std::max({std::abs(weightedDose), std::abs(doseFromEnergy), std::abs(runDose)});
  if (!std::isfinite(weightedDose) || !std::isfinite(doseFromEnergy)
      || !std::isfinite(runDose)
      || (scale > 0. && (std::abs(weightedDose - doseFromEnergy) > 1.e-10 * scale
                         || std::abs(runDose - doseFromEnergy) > 1.e-10 * scale))) {
    G4ExceptionDescription msg;
    msg << "Mass-weighted region dose " << weightedDose / gray
        << " Gy or accumulated phantom dose " << runDose / gray
        << " Gy does not match total energy / water mass " << doseFromEnergy / gray << " Gy.";
    G4Exception("SourceLabRunAction::EndOfRunAction", "SourceLabScoring004", FatalException, msg);
    return;
  }
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
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 9, G4Version);
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 10, G4Random::getTheEngine()->name());
    analysisManager->FillNtupleSColumn(fRunInfoNtupleId, 11, run->GetRandomNumberStatus());
    analysisManager->AddNtupleRow(fRunInfoNtupleId);
  }
  if (writeRunRow) {
    fScoreStatistics.WriteRows(fStatisticsNtupleId, -1);
    for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
      fRegionStatistics[i].WriteRows(fStatisticsNtupleId, static_cast<G4int>(i));
    }
  }
  analysisManager->Write();
  analysisManager->CloseFile(false);

  if (finalizeOutput) {
    G4cout << "Run complete: events=" << run->GetNumberOfEvent()
           << ", file=" << BuildOutputFileName()
           << ", source=" << fOutputSource
           << ", geometry=" << fOutputGeometry
           << ", region=" << fOutputRegion
           << ", depth_cm=" << fOutputDepthCm
           << ", em=" << fEmModel
           << ", decay=" << (fEnableRadioactiveDecay ? "on" : "off")
           << G4endl;

    G4cout << "Run summary: " << run->GetNumberOfEvent() << " events" << G4endl;
    G4cout << "Total dose in phantom: " << ToScientific(GetRunDose(), gray, "Gy") << G4endl;
    G4cout << "Mass-weighted region dose: " << ToScientific(weightedDose, gray, "Gy")
           << " (consistent with total energy / water mass)" << G4endl;
    G4cout << "Total energy deposit in phantom: " << ToScientific(GetRunEnergyDeposit(), eV, "eV") << G4endl;
    G4cout << "Total charged track length in phantom: " << ToScientific(GetRunTrackLength(), mm, "mm") << G4endl;
    fScoreStatistics.Print("phantom");
    for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
      G4cout << "Water region " << i << " [" << regions[i].innerRadius / mm << ", "
             << regions[i].outerRadius / mm << "] mm: dose="
             << ToScientific(fRegionDoses[i].GetValue(), gray, "Gy")
             << ", edep=" << ToScientific(fRegionEnergyDeposits[i].GetValue(), eV, "eV")
             << G4endl;
      fRegionStatistics[i].Print("water region " + std::to_string(i));
    }
  }
}

void SourceLabRunAction::AddEventScoring(const WaterScores& doses, const WaterScores& energyDeposits,
                                         const WaterScores& trackLengths)
{
  const auto& regions = GetWaterRegions();
  G4double totalMass = 0.;
  G4double eventEnergy = 0.;
  G4double eventTrackLength = 0.;
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    totalMass += regions[i].mass;
    eventEnergy += energyDeposits[i];
    eventTrackLength += trackLengths[i];
    fRegionDoses[i] += doses[i];
    fRegionEnergyDeposits[i] += energyDeposits[i];
    fRegionTrackLengths[i] += trackLengths[i];
    fRegionStatistics[i].Add(doses[i], energyDeposits[i], trackLengths[i]);
  }
  fRunDose += eventEnergy / totalMass;
  fRunEnergyDeposit += eventEnergy;
  fRunTrackLength += eventTrackLength;
  fScoreStatistics.Add(eventEnergy / totalMass, eventEnergy, eventTrackLength);
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

G4int SourceLabRunAction::GetPhantomNtupleId() const
{
  return fPhantomNtupleId;
}

G4int SourceLabRunAction::GetWaterRegionNtupleId() const
{
  return fWaterRegionNtupleId;
}

const std::array<WaterRegion, kWaterRegionCount>& SourceLabRunAction::GetWaterRegions() const
{
  if (!fDetectorConstruction) {
    G4Exception("SourceLabRunAction::GetWaterRegions", "SourceLabScoring005", FatalException,
                "Detector construction is required for water scoring.");
  }
  return fDetectorConstruction->GetWaterRegions();
}

}  // namespace SourceLab
