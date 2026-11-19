#ifndef SourceLabRunAction_h
#define SourceLabRunAction_h 1

#include "G4Accumulable.hh"
#include "G4GenericMessenger.hh"
#include "G4UserRunAction.hh"
#include "globals.hh"

#include "SourceLabDetectorConstruction.hh"
#include "ScoreStatistics.hh"

#include <array>
#include <memory>

namespace SourceLab
{
using WaterScores = std::array<G4double, kWaterRegionCount>;

class SourceLabRunAction : public G4UserRunAction
{
  public:
    explicit SourceLabRunAction(SourceLabDetectorConstruction* detectorConstruction = nullptr,
      const G4String& emModel = "option4",
      G4bool enableRadioactiveDecay = false);
    ~SourceLabRunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
    void AddEventScoring(const WaterScores& doses, const WaterScores& energyDeposits,
                         const WaterScores& trackLengths);
    G4double GetRunDose() const;
    G4double GetRunEnergyDeposit() const;
    G4double GetRunTrackLength() const;
    G4int GetPhantomNtupleId() const;
    G4int GetWaterRegionNtupleId() const;
    const std::array<WaterRegion, kWaterRegionCount>& GetWaterRegions() const;

  private:
    void ConfigureCommands();
    void SetOutputTag(const G4String& tag);
    void SetOutputSource(const G4String& source);
    void SetOutputDepth(G4double depth);
    G4String BuildOutputFileName() const;
    static G4String SanitizeForFileName(const G4String& value);

    SourceLabDetectorConstruction* fDetectorConstruction = nullptr;
    G4String fEmModel;
    G4bool fEnableRadioactiveDecay = false;
    G4Accumulable<G4double> fRunDose = 0.0;
    G4Accumulable<G4double> fRunEnergyDeposit = 0.0;
    G4Accumulable<G4double> fRunTrackLength = 0.0;
    std::array<G4Accumulable<G4double>, kWaterRegionCount> fRegionDoses{};
    std::array<G4Accumulable<G4double>, kWaterRegionCount> fRegionEnergyDeposits{};
    std::array<G4Accumulable<G4double>, kWaterRegionCount> fRegionTrackLengths{};
    ScoreStatistics fScoreStatistics;
    std::array<ScoreStatistics, kWaterRegionCount> fRegionStatistics{};
    G4int fStatisticsNtupleId = -1;
    G4int fPhantomNtupleId = -1;
    G4int fRunNtupleId = -1;
    G4int fRunInfoNtupleId = -1;
    G4int fWaterRegionNtupleId = -1;
    G4int fWaterRegionRunNtupleId = -1;

    std::unique_ptr<G4GenericMessenger> fMessenger;
    G4String fOutputTag = "default";
    G4String fOutputSource = "unspecified";
    G4String fOutputGeometry = "water-annulus";
    G4String fOutputRegion = "phantom";
    G4double fOutputDepthCm = 0.;
};

}  // namespace SourceLab

#endif
