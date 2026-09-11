#include "SourceLabDetectorConstruction.hh"
#include "SourceLabDetectorMessenger.hh"

#include <cmath>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4NistManager.hh"
#include "G4PSDoseDeposit.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PSTrackLength.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4RotationMatrix.hh"
#include "G4SDChargedFilter.hh"
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
  auto* sampleDetector = new G4MultiFunctionalDetector("Sample");
  G4SDManager::GetSDMpointer()->AddNewDetector(sampleDetector);

  auto* doseScorer = new G4PSDoseDeposit("Dose");
  sampleDetector->RegisterPrimitive(doseScorer);

  auto* energyScorer = new G4PSEnergyDeposit("Edep");
  sampleDetector->RegisterPrimitive(energyScorer);

  auto* trackLengthScorer = new G4PSTrackLength("TrackL");
  auto* charged = new G4SDChargedFilter("chargedFilter");
  trackLengthScorer->SetFilter(charged);
  sampleDetector->RegisterPrimitive(trackLengthScorer);

  auto* capDetector = new G4MultiFunctionalDetector("SampleEndCap");
  G4SDManager::GetSDMpointer()->AddNewDetector(capDetector);

  auto* capDoseScorer = new G4PSDoseDeposit("Dose");
  capDetector->RegisterPrimitive(capDoseScorer);

  auto* capEnergyScorer = new G4PSEnergyDeposit("Edep");
  capDetector->RegisterPrimitive(capEnergyScorer);

  auto* capTrackLengthScorer = new G4PSTrackLength("TrackL");
  auto* capCharged = new G4SDChargedFilter("chargedFilter");
  capTrackLengthScorer->SetFilter(capCharged);
  capDetector->RegisterPrimitive(capTrackLengthScorer);

  auto* depthDetector = new G4MultiFunctionalDetector("SampleDepth");
  G4SDManager::GetSDMpointer()->AddNewDetector(depthDetector);

  auto* depthDoseScorer = new G4PSDoseDeposit("Dose");
  depthDetector->RegisterPrimitive(depthDoseScorer);

  auto* depthEnergyScorer = new G4PSEnergyDeposit("Edep");
  depthDetector->RegisterPrimitive(depthEnergyScorer);

  auto* depthTrackLengthScorer = new G4PSTrackLength("TrackL");
  auto* depthCharged = new G4SDChargedFilter("chargedFilter");
  depthTrackLengthScorer->SetFilter(depthCharged);
  depthDetector->RegisterPrimitive(depthTrackLengthScorer);

  SetSensitiveDetector("Sample", sampleDetector);
  SetSensitiveDetector("SampleEndCap", capDetector);
  SetSensitiveDetector("SampleDepth", depthDetector);
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

void SourceLabDetectorConstruction::SetSampleSkinThickness(G4double thickness)
{
  if (thickness >= 0.) {
    fConfig.sampleSkinThickness = thickness;
  }
}

void SourceLabDetectorConstruction::SetSampleEndCapArea(G4double area)
{
  if (area > 0.) {
    fConfig.sampleEndCapTargetArea = area;
  }
}

void SourceLabDetectorConstruction::SetSampleEndCapThickness(G4double thickness)
{
  if (thickness >= 0.) {
    fConfig.sampleEndCapThickness = thickness;
  }
}

void SourceLabDetectorConstruction::SetSampleAxis(const G4String& axisName)
{
  const G4String lowered = axisName;
  if (lowered == "x" || lowered == "X") {
    fConfig.sampleAxis = SampleAxis::kX;
  }
  else if (lowered == "y" || lowered == "Y") {
    fConfig.sampleAxis = SampleAxis::kY;
  }
  else {
    fConfig.sampleAxis = SampleAxis::kZ;
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

G4double SourceLabDetectorConstruction::GetSampleSkinThickness() const
{
  return fConfig.sampleSkinThickness;
}

G4double SourceLabDetectorConstruction::GetSampleEndCapArea() const
{
  return fConfig.sampleEndCapTargetArea;
}

G4double SourceLabDetectorConstruction::GetSampleEndCapThickness() const
{
  return fConfig.sampleEndCapThickness;
}

G4double SourceLabDetectorConstruction::ComputeSampleEndCapRadius() const
{
  const G4double shellRadius = fConfig.sampleRadius + fConfig.sampleSkinThickness;
  if (shellRadius <= 0.) {
    return 0.;
  }

  const G4double targetRadius = std::sqrt(fConfig.sampleEndCapTargetArea / pi);
  if (targetRadius < shellRadius) {
    return targetRadius;
  }

  return 0.5 * shellRadius;
}

G4String SourceLabDetectorConstruction::GetSampleAxis() const
{
  switch (fConfig.sampleAxis) {
    case SampleAxis::kX:
      return "x";
    case SampleAxis::kY:
      return "y";
    case SampleAxis::kZ:
    default:
      return "z";
  }
}

G4RotationMatrix* SourceLabDetectorConstruction::BuildSampleRotation() const
{
  if (fConfig.sampleAxis == SampleAxis::kZ) {
    return nullptr;
  }

  auto* rotation = new G4RotationMatrix();
  if (fConfig.sampleAxis == SampleAxis::kX) {
    rotation->rotateY(90. * deg);
  }
  else if (fConfig.sampleAxis == SampleAxis::kY) {
    rotation->rotateX(-90. * deg);
  }

  return rotation;
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
  G4cout << "  sampleSkinThickness = " << fConfig.sampleSkinThickness / mm << " mm" << G4endl;
  G4cout << "  sampleEndCapTargetArea = " << fConfig.sampleEndCapTargetArea / (cm * cm) << " cm2" << G4endl;
  G4cout << "  sampleEndCapThickness = " << fConfig.sampleEndCapThickness / mm << " mm" << G4endl;
  G4cout << "  sampleAxis = " << GetSampleAxis() << G4endl;
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

  G4double sampleOuterRadius = fConfig.sampleRadius + fConfig.sampleSkinThickness;
  G4double sampleShellHalfThickness = fConfig.sampleThickness / 2.0 + fConfig.sampleSkinThickness;

  auto* sampleShellSolid = new G4Tubs("SampleShell", 0., sampleOuterRadius, sampleShellHalfThickness, 0., 2. * pi);
  auto* sampleShellLogic = new G4LogicalVolume(sampleShellSolid, sampleMat, "SampleShell");

  auto* sampleSolid = new G4Tubs("Sample", 0., fConfig.sampleRadius, fConfig.sampleThickness / 2.0, 0., 2. * pi);
  auto* sampleLogic = new G4LogicalVolume(sampleSolid, sampleMat, "Sample");
  fSampleLog = sampleLogic;

  G4double capRadius = ComputeSampleEndCapRadius();
  G4double capThickness = std::max(fConfig.sampleEndCapThickness, 0.25 * mm);
  G4double capHalfThickness = capThickness / 2.0;
  auto* sampleEndCapSolid = new G4Tubs("SampleEndCap", 0., capRadius, capHalfThickness, 0., 2. * pi);
  auto* sampleEndCapLogic = new G4LogicalVolume(sampleEndCapSolid, sampleMat, "SampleEndCap");

  G4double depthRadius = capRadius;
  G4double depthThickness = capThickness;
  G4double depthHalfThickness = depthThickness / 2.0;
  auto* sampleDepthSolid = new G4Tubs("SampleDepth", 0., depthRadius, depthHalfThickness, 0., 2. * pi);
  auto* sampleDepthLogic = new G4LogicalVolume(sampleDepthSolid, sampleMat, "SampleDepth");

  G4double maxDepthInset = 0.5 * fConfig.sampleThickness - depthHalfThickness;
  G4double depthOffset = std::min(1.0 * cm, std::max(0.0, maxDepthInset));

  G4double sampleCenterZ = -fConfig.phantomHalfZ + fConfig.sampleDepth + sampleShellHalfThickness;
  new G4PVPlacement(BuildSampleRotation(), G4ThreeVector(0., 0., sampleCenterZ), sampleShellLogic, "SampleShell", phantomLogic, false, 0, fCheckOverlaps);
  new G4PVPlacement(BuildSampleRotation(), G4ThreeVector(0., 0., -sampleCenterZ), sampleShellLogic, "SampleShellMirror", phantomLogic, false, 1, fCheckOverlaps);
  new G4PVPlacement(nullptr, G4ThreeVector(), sampleLogic, "Sample", sampleShellLogic, false, 0, fCheckOverlaps);
  new G4PVPlacement(nullptr,
                    G4ThreeVector(0., 0., sampleShellHalfThickness - capHalfThickness),
                    sampleEndCapLogic,
                    "SampleEndCap",
                    sampleShellLogic,
                    false,
                    0,
                    fCheckOverlaps);
  new G4PVPlacement(nullptr,
                    G4ThreeVector(0., 0., depthOffset),
                    sampleDepthLogic,
                    "SampleDepth",
                    sampleLogic,
                    false,
                    0,
                    fCheckOverlaps);

  worldLogic->SetVisAttributes(G4VisAttributes::GetInvisible());

  auto* sampleVis = new G4VisAttributes(G4Colour(0.9, 0.8, 0.3, 0.4));
  sampleVis->SetForceSolid(true);
  sampleLogic->SetVisAttributes(sampleVis);

  auto* sampleEndCapVis = new G4VisAttributes(G4Colour(0.8, 0.2, 0.2, 0.8));
  sampleEndCapVis->SetForceSolid(true);
  sampleEndCapLogic->SetVisAttributes(sampleEndCapVis);

  auto* sampleDepthVis = new G4VisAttributes(G4Colour(0.2, 0.8, 0.4, 0.8));
  sampleDepthVis->SetForceSolid(true);
  sampleDepthLogic->SetVisAttributes(sampleDepthVis);

  auto* sampleShellVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.6));
  sampleShellVis->SetForceSolid(false);
  sampleShellVis->SetForceWireframe(true);
  sampleShellLogic->SetVisAttributes(sampleShellVis);

  auto* phantomVis = new G4VisAttributes(G4Colour(0.2, 0.5, 1.0, 0.2));
  phantomVis->SetForceSolid(false);
  phantomVis->SetForceWireframe(true);
  phantomLogic->SetVisAttributes(phantomVis);

  return worldPhys;
}

}  // namespace SourceLab
