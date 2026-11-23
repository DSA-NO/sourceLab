#include "SourceLabDetectorConstruction.hh"
#include "SourceLabDetectorMessenger.hh"

#include <string>

#include "G4Box.hh"
#include "G4Element.hh"
#include "G4Exception.hh"
#include "G4Isotope.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4NistManager.hh"
#include "G4PSDoseDeposit.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PSTrackLength.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4SDChargedFilter.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"

namespace SourceLab
{
namespace
{
const G4double kLayerThickness = 0.07 * mm;
const G4double kOuterLayerDepth = 30. * mm;
}

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
  nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
  nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

  auto* cesium137 = new G4Isotope("Cs137", 55, 137, 136.90708923 * g / mole);
  auto* cesium137Element = new G4Element("Cesium-137", "Cs137", 1);
  cesium137Element->AddIsotope(cesium137, 100. * perCent);
  auto* cesium137Metal = new G4Material("Cs137Metal", 1.873 * g / cm3, 1, kStateSolid);
  cesium137Metal->AddElement(cesium137Element, 1);
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
  auto* phantomDetector = new G4MultiFunctionalDetector("Phantom");
  G4SDManager::GetSDMpointer()->AddNewDetector(phantomDetector);

  auto* doseScorer = new G4PSDoseDeposit("Dose");
  phantomDetector->RegisterPrimitive(doseScorer);

  auto* energyScorer = new G4PSEnergyDeposit("Edep");
  phantomDetector->RegisterPrimitive(energyScorer);

  auto* trackLengthScorer = new G4PSTrackLength("TrackL");
  auto* charged = new G4SDChargedFilter("chargedFilter");
  trackLengthScorer->SetFilter(charged);
  phantomDetector->RegisterPrimitive(trackLengthScorer);

  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    SetSensitiveDetector("Phantom" + std::to_string(i), phantomDetector);
  }
}

void SourceLabDetectorConstruction::SetWorldSize(G4double worldSize)
{
  if (worldSize > 0.) {
    fConfig.worldSize = worldSize;
  }
}

void SourceLabDetectorConstruction::SetPhantomHeight(G4double height)
{
  if (height > 0.) {
    fConfig.phantomHeight = height;
  }
}

void SourceLabDetectorConstruction::SetSourceRadius(G4double radius)
{
  if (radius > 0.) {
    fConfig.sourceRadius = radius;
  }
}

void SourceLabDetectorConstruction::SetSourceShellThickness(G4double thickness)
{
  if (thickness > 0.) {
    fConfig.sourceShellThickness = thickness;
  }
}

void SourceLabDetectorConstruction::SetPhantomMaterial(const G4String& material)
{
  if (material != "G4_WATER" && material != "G4_TISSUE_SOFT_ICRP") {
    G4ExceptionDescription message;
    message << "Unsupported phantom material '" << material << "'.";
    G4Exception("SourceLabDetectorConstruction::SetPhantomMaterial", "SourceLabGeometry002",
                FatalException, message);
    return;
  }
  fConfig.phantomMaterial = material;
}

G4double SourceLabDetectorConstruction::GetWorldSize() const
{
  return fConfig.worldSize;
}

const std::array<WaterRegion, kWaterRegionCount>& SourceLabDetectorConstruction::GetWaterRegions() const
{
  return fWaterRegions;
}

void SourceLabDetectorConstruction::PrintConfig() const
{
  G4cout << "SourceLab detector configuration:" << G4endl;
  G4cout << "  worldSize = " << fConfig.worldSize / cm << " cm" << G4endl;
  G4cout << "  phantomRadius = "
         << (fConfig.sourceRadius + fConfig.sourceShellThickness + kOuterLayerDepth + kLayerThickness / 2.) / cm
         << " cm" << G4endl;
  G4cout << "  phantomHeight = " << fConfig.phantomHeight / cm << " cm" << G4endl;
  G4cout << "  phantomMaterial = " << fConfig.phantomMaterial << G4endl;
  G4cout << "  sourceRadius = " << fConfig.sourceRadius / mm << " mm" << G4endl;
  G4cout << "  sourceHeight = " << fConfig.phantomHeight / cm << " cm" << G4endl;
  G4cout << "  sourceShellThickness = " << fConfig.sourceShellThickness / mm << " mm" << G4endl;
}

G4VPhysicalVolume* SourceLabDetectorConstruction::DefineVolumes()
{
  auto* worldMat = G4Material::GetMaterial("G4_AIR");
  if (fConfig.phantomMaterial != "G4_WATER" && fConfig.phantomMaterial != "G4_TISSUE_SOFT_ICRP") {
    G4ExceptionDescription message;
    message << "Unsupported phantom material '" << fConfig.phantomMaterial << "'.";
    G4Exception("SourceLabDetectorConstruction::DefineVolumes", "SourceLabGeometry002",
                FatalException, message);
    return nullptr;
  }
  auto* phantomMat = G4Material::GetMaterial(fConfig.phantomMaterial, false);
  if (!phantomMat) {
    G4ExceptionDescription message;
    message << "Phantom material '" << fConfig.phantomMaterial << "' is unavailable.";
    G4Exception("SourceLabDetectorConstruction::DefineVolumes", "SourceLabGeometry003",
                FatalException, message);
    return nullptr;
  }
  auto* sourceMat = G4Material::GetMaterial("Cs137Metal");
  auto* shellMat = G4Material::GetMaterial("G4_STAINLESS-STEEL");

  const G4double shellOuterRadius = fConfig.sourceRadius + fConfig.sourceShellThickness;
  const std::array<G4double, kWaterRegionCount + 1> boundaries = {
    shellOuterRadius,
    shellOuterRadius + kLayerThickness,
    shellOuterRadius + 10. * mm - kLayerThickness / 2.,
    shellOuterRadius + 10. * mm + kLayerThickness / 2.,
    shellOuterRadius + kOuterLayerDepth - kLayerThickness / 2.,
    shellOuterRadius + kOuterLayerDepth + kLayerThickness / 2.
  };
  if (fConfig.worldSize <= 0.
      || fConfig.phantomHeight <= 0.
      || fConfig.sourceRadius <= 0.
      || fConfig.sourceShellThickness <= 0.
      || 2.0 * boundaries.back() > fConfig.worldSize
      || fConfig.phantomHeight > fConfig.worldSize) {
    G4ExceptionDescription message;
    message << "Geometry dimensions must be positive, and the phantom must fit inside the world.";
    G4Exception("SourceLabDetectorConstruction::DefineVolumes", "SourceLabGeometry001", FatalException, message);
    return nullptr;
  }

  auto* worldSolid = new G4Box("World", fConfig.worldSize / 2.0, fConfig.worldSize / 2.0, fConfig.worldSize / 2.0);
  auto* worldLogic = new G4LogicalVolume(worldSolid, worldMat, "World");
  auto* worldPhys = new G4PVPlacement(nullptr, G4ThreeVector(), worldLogic, "World", nullptr, false, 0, fCheckOverlaps);

  auto* sourceSolid = new G4Tubs("Source", 0., fConfig.sourceRadius, fConfig.phantomHeight / 2.0, 0., twopi);
  auto* sourceLogic = new G4LogicalVolume(sourceSolid, sourceMat, "Source");
  new G4PVPlacement(nullptr, G4ThreeVector(), sourceLogic, "Source", worldLogic, false, 0, fCheckOverlaps);

  auto* sourceShellSolid = new G4Tubs("SourceShell",
                                      fConfig.sourceRadius,
                                      shellOuterRadius,
                                      fConfig.phantomHeight / 2.0,
                                      0.,
                                      twopi);
  auto* sourceShellLogic = new G4LogicalVolume(sourceShellSolid, shellMat, "SourceShell");
  new G4PVPlacement(nullptr, G4ThreeVector(), sourceShellLogic, "SourceShell", worldLogic, false, 0, fCheckOverlaps);

  std::array<G4LogicalVolume*, kWaterRegionCount> phantomLogic{};
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    const auto name = "Phantom" + std::to_string(i);
    auto* solid = new G4Tubs(name, boundaries[i], boundaries[i + 1], fConfig.phantomHeight / 2.0, 0., twopi);
    phantomLogic[i] = new G4LogicalVolume(solid, phantomMat, name);
    new G4PVPlacement(nullptr, G4ThreeVector(), phantomLogic[i], name, worldLogic, false,
                      static_cast<G4int>(i), fCheckOverlaps);
    fWaterRegions[i] = {boundaries[i], boundaries[i + 1], solid->GetCubicVolume() * phantomMat->GetDensity()};
  }

  worldLogic->SetVisAttributes(G4VisAttributes::GetInvisible());

  auto* sourceVis = new G4VisAttributes(G4Colour(0.9, 0.75, 0.25, 0.8));
  sourceVis->SetForceSolid(true);
  sourceLogic->SetVisAttributes(sourceVis);

  auto* sourceShellVis = new G4VisAttributes(G4Colour(0.55, 0.58, 0.62, 0.55));
  sourceShellVis->SetForceSolid(true);
  sourceShellLogic->SetVisAttributes(sourceShellVis);

  auto* phantomVis = new G4VisAttributes(G4Colour(0.2, 0.5, 1.0, 0.2));
  phantomVis->SetForceWireframe(true);
  auto* layerVis = new G4VisAttributes(G4Colour(0.2, 0.8, 0.4, 0.6));
  layerVis->SetForceSolid(true);
  for (std::size_t i = 0; i < kWaterRegionCount; ++i) {
    phantomLogic[i]->SetVisAttributes(i % 2 == 0 ? layerVis : phantomVis);
  }

  return worldPhys;
}

}  // namespace SourceLab
