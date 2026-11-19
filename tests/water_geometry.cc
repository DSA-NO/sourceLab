#include "SourceLabDetectorConstruction.hh"

#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4Navigator.hh"
#include "G4PhysicalConstants.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
void Check(bool condition)
{
  if (!condition) throw std::runtime_error("water geometry regression");
}

bool Close(G4double a, G4double b)
{
  return std::abs(a - b) <= 1.e-9 * std::max(std::abs(a), std::abs(b));
}
}

int main(int argc, char** argv)
{
  SourceLab::SourceLabDetectorConstruction detector;
  auto config = detector.GetConfig();
  if (argc > 1 && std::string(argv[1]) == "custom") {
    config.sourceRadius = 3. * mm;
    config.sourceShellThickness = 1.25 * mm;
    detector.SetConfig(config);
  }
  auto* world = detector.Construct();
  const auto wall = config.sourceRadius + config.sourceShellThickness;
  const std::array<G4double, 6> boundaries{
    wall, wall + 0.07 * mm, wall + 9.965 * mm,
    wall + 10.035 * mm, wall + 29.965 * mm, wall + 30.035 * mm
  };
  const auto& regions = detector.GetWaterRegions();
  Check(regions.size() == 5);
  G4double totalMass = 0.;
  for (std::size_t i = 0; i < regions.size(); ++i) {
    const auto* volume = G4PhysicalVolumeStore::GetInstance()->GetVolume("Phantom" + std::to_string(i));
    Check(volume && volume->GetCopyNo() == static_cast<G4int>(i));
    const auto* solid = dynamic_cast<const G4Tubs*>(volume->GetLogicalVolume()->GetSolid());
    Check(solid && volume->GetLogicalVolume()->GetMaterial()->GetName() == "G4_WATER");
    Check(Close(regions[i].innerRadius, boundaries[i]) && Close(regions[i].outerRadius, boundaries[i + 1]));
    Check(Close(solid->GetInnerRadius(), boundaries[i]) && Close(solid->GetOuterRadius(), boundaries[i + 1]));
    Check(Close(2. * solid->GetZHalfLength(), config.phantomHeight));
    totalMass += regions[i].mass;
  }
  const auto expectedMass = pi * (boundaries.back() * boundaries.back() - wall * wall)
                            * config.phantomHeight * (1. * g / cm3);
  Check(Close(totalMass, expectedMass));

  G4Navigator navigator;
  navigator.SetWorldVolume(world);
  auto* inside = navigator.LocateGlobalPointAndSetup(G4ThreeVector(boundaries.back() - 0.01 * mm, 0., 0.));
  Check(inside && inside->GetName() == "Phantom4");
  auto* outside = navigator.LocateGlobalPointAndSetup(G4ThreeVector(boundaries.back() + 0.01 * mm, 0., 0.));
  Check(outside && outside->GetName() == "World");
}
