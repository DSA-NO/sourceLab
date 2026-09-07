#include "SourceLabDetectorConstruction.hh"
#include "SourceLabDetectorMessenger.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4NistManager.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"

namespace SourceLab
{

SourceLabDetectorConstruction::SourceLabDetectorConstruction()
{
  DefineMaterials();
  fMessenger = new SourceLabDetectorMessenger(this);
}

SourceLabDetectorConstruction::~SourceLabDetectorConstruction()
{
  delete fMessenger;
}

void SourceLabDetectorConstruction::DefineMaterials()
{
  auto* nist = G4NistManager::Instance();
  nist->FindOrBuildMaterial("G4_AIR");
  nist->FindOrBuildMaterial("G4_WATER");
  nist->FindOrBuildMaterial("G4_PLEXIGLASS");
}

void SourceLabDetectorConstruction::SetConfig(const DetectorConfig& config)
{
  fConfig = config;
}

DetectorConfig SourceLabDetectorConstruction::GetConfig() const
{
  return fConfig;
}

G4VPhysicalVolume* SourceLabDetectorConstruction::Construct()
{
  return DefineVolumes();
}

void SourceLabDetectorConstruction::ConstructSDandField()
{
}

void SourceLabDetectorConstruction::SetWorldSize(G4double worldSize)
{
  if (worldSize > 0.) {
    fConfig.worldSize = worldSize;
  }
}

void SourceLabDetectorConstruction::SetPhantomHalfSize(G4double x, G4double y, G4double z)
{
  if (x > 0.)
    fConfig.phantomHalfX = x;
  if (y > 0.)
    fConfig.phantomHalfY = y;
  if (z > 0.)
    fConfig.phantomHalfZ = z;
}

void SourceLabDetectorConstruction::SetSampleDepth(G4double depth)
{
  if (depth > 0.) {
    fConfig.sampleDepth = depth;
  }
}

void SourceLabDetectorConstruction::SetSampleSize(G4double radius, G4double thickness)
{
  if (radius > 0.) {
    fConfig.sampleRadius = radius;
  }
  if (thickness > 0.) {
    fConfig.sampleThickness = thickness;
  }
}

G4double SourceLabDetectorConstruction::GetWorldSize() const
{
  return fConfig.worldSize;
}

G4double SourceLabDetectorConstruction::GetSampleDepth() const
{
  return fConfig.sampleDepth;
}

G4double SourceLabDetectorConstruction::GetSampleRadius() const
{
  return fConfig.sampleRadius;
}

G4double SourceLabDetectorConstruction::GetSampleThickness() const
{
  return fConfig.sampleThickness;
}

void SourceLabDetectorConstruction::PrintConfig() const
{
  G4cout << "SourceLab detector configuration:" << G4endl;
  G4cout << "  worldSize = " << fConfig.worldSize / m << " m" << G4endl;
  G4cout << "  phantomHalf = (" << fConfig.phantomHalfX / m << ", " << fConfig.phantomHalfY / m << ", "
         << fConfig.phantomHalfZ / m << ") m" << G4endl;
  G4cout << "  sampleDepth = " << fConfig.sampleDepth / cm << " cm" << G4endl;
  G4cout << "  sampleRadius = " << fConfig.sampleRadius / cm << " cm" << G4endl;
  G4cout << "  sampleThickness = " << fConfig.sampleThickness / mm << " mm" << G4endl;
}

G4VPhysicalVolume* SourceLabDetectorConstruction::DefineVolumes()
{
  auto* worldMat = G4Material::GetMaterial("G4_AIR");
  auto* phantomMat = G4Material::GetMaterial("G4_WATER");
  auto* sampleMat = G4Material::GetMaterial("G4_PLEXIGLASS");

  auto* worldSolid = new G4Box("World", fConfig.worldSize / 2.0, fConfig.worldSize / 2.0, fConfig.worldSize / 2.0);
  auto* worldLogic = new G4LogicalVolume(worldSolid, worldMat, "World");
  auto* worldPhys = new G4PVPlacement(nullptr, G4ThreeVector(), worldLogic, "World", nullptr, false, 0, fCheckOverlaps);

  auto* phantomSolid = new G4Box("Phantom", fConfig.phantomHalfX, fConfig.phantomHalfY, fConfig.phantomHalfZ);
  auto* phantomLogic = new G4LogicalVolume(phantomSolid, phantomMat, "Phantom");
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), phantomLogic, "Phantom", worldLogic, false, 0, fCheckOverlaps);

  auto* sampleSolid = new G4Tubs("Sample", 0., fConfig.sampleRadius, fConfig.sampleThickness / 2.0, 0., 2. * pi);
  auto* sampleLogic = new G4LogicalVolume(sampleSolid, sampleMat, "Sample");
  fSampleLog = sampleLogic;

  auto* sampleSD = new G4MultiFunctionalDetector("SampleSD");
  auto* energyScorer = new G4PSEnergyDeposit("eDep");
  sampleSD->RegisterPrimitive(energyScorer);
  G4SDManager::GetSDMpointer()->AddNewDetector(sampleSD);
  sampleLogic->SetSensitiveDetector(sampleSD);

  G4double sampleCenterZ = -fConfig.phantomHalfZ + fConfig.sampleDepth + fConfig.sampleThickness / 2.0;
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., sampleCenterZ), sampleLogic, "Sample", phantomLogic, false, 0, fCheckOverlaps);

  auto* sampleVis = new G4VisAttributes(G4Colour(0.9, 0.8, 0.3, 0.4));
  sampleVis->SetForceSolid(true);
  sampleLogic->SetVisAttributes(sampleVis);

  auto* phantomVis = new G4VisAttributes(G4Colour(0.2, 0.5, 1.0, 0.2));
  phantomVis->SetForceSolid(true);
  phantomLogic->SetVisAttributes(phantomVis);

  return worldPhys;
}

}  // namespace SourceLab
