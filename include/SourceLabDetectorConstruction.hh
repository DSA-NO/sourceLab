#ifndef SourceLabDetectorConstruction_h
#define SourceLabDetectorConstruction_h 1

#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

#include <array>

class G4VPhysicalVolume;

namespace SourceLab
{
class SourceLabDetectorMessenger;

constexpr std::size_t kWaterRegionCount = 5;

struct WaterRegion
{
  G4double innerRadius = 0.;
  G4double outerRadius = 0.;
  G4double mass = 0.;
};

struct DetectorConfig
{
  G4double worldSize = 30.0 * cm;
  G4double phantomHeight = 5.0 * cm;
  G4double sourceRadius = 2.375 * mm;
  G4double sourceShellThickness = 0.8 * mm;
};

class SourceLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    SourceLabDetectorConstruction();
    ~SourceLabDetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    void SetConfig(const DetectorConfig& config);
    DetectorConfig GetConfig() const;

    void SetWorldSize(G4double worldSize);
    void SetPhantomHeight(G4double height);
    void SetSourceRadius(G4double radius);
    void SetSourceShellThickness(G4double thickness);

    G4double GetWorldSize() const;
    const std::array<WaterRegion, kWaterRegionCount>& GetWaterRegions() const;
    void PrintConfig() const;

  private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    SourceLabDetectorMessenger* fMessenger = nullptr;
    G4bool fCheckOverlaps = true;

    DetectorConfig fConfig;
    std::array<WaterRegion, kWaterRegionCount> fWaterRegions{};
};

}  // namespace SourceLab

#endif
