#ifndef SourceLabDetectorConstruction_h
#define SourceLabDetectorConstruction_h 1

#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

namespace SourceLab
{
class SourceLabDetectorMessenger;

enum class SampleAxis
{
  kZ,
  kX,
  kY
};

struct DetectorConfig
{
  G4double worldSize = 2.0 * m;
  G4double phantomHalfX = 0.5 * m;
  G4double phantomHalfY = 0.5 * m;
  G4double phantomHalfZ = 0.5 * m;
  G4double sampleDepth = 10.0 * cm;
  G4double sampleRadius = 5.0 * cm;
  G4double sampleThickness = 1.0 * mm;
  G4double sampleSkinThickness = 0.25 * mm;
  G4double sampleEndCapTargetArea = 1.0 * cm * 1.0 * cm;
  G4double sampleEndCapThickness = 0.25 * mm;
  SampleAxis sampleAxis = SampleAxis::kZ;
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
    void SetPhantomHalfSize(G4double x, G4double y, G4double z);
    void SetSampleDepth(G4double depth);
    void SetSampleSize(G4double radius, G4double thickness);
    void SetSampleSkinThickness(G4double thickness);
    void SetSampleEndCapArea(G4double area);
    void SetSampleEndCapThickness(G4double thickness);
    void SetSampleAxis(const G4String& axisName);

    G4double GetWorldSize() const;
    G4double GetSampleDepth() const;
    G4double GetSampleRadius() const;
    G4double GetSampleThickness() const;
    G4double GetSampleSkinThickness() const;
    G4double GetSampleEndCapArea() const;
    G4double GetSampleEndCapThickness() const;
    G4double ComputeSampleEndCapRadius() const;
    G4String GetSampleAxis() const;
    G4RotationMatrix* BuildSampleRotation() const;
    void PrintConfig() const;

  private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    SourceLabDetectorMessenger* fMessenger = nullptr;
    G4bool fCheckOverlaps = true;

    DetectorConfig fConfig;
    G4LogicalVolume* fSampleLog = nullptr;
};

}  // namespace SourceLab

#endif
