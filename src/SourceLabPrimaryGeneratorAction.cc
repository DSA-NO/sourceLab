#include "SourceLabPrimaryGeneratorAction.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4SingleParticleSource.hh"
#include "G4SystemOfUnits.hh"

namespace SourceLab
{

SourceLabPrimaryGeneratorAction::SourceLabPrimaryGeneratorAction(
  SourceLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
  fParticleSource = new G4GeneralParticleSource();

  auto* source = fParticleSource->GetCurrentSource();
  source->GetPosDist()->SetPosDisType("Point");
  source->GetPosDist()->SetCentreCoords(G4ThreeVector(0., 0., -0.75 * m));

  source->GetAngDist()->SetAngDistType("iso");
  source->GetAngDist()->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));

  source->GetEneDist()->SetEnergyDisType("Mono");
  source->GetEneDist()->SetMonoEnergy(1.25 * MeV);

  auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  if (particle) {
    source->SetParticleDefinition(particle);
  }
}

SourceLabPrimaryGeneratorAction::~SourceLabPrimaryGeneratorAction()
{
  delete fParticleSource;
}

void SourceLabPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  if (!fParticleSource) {
    return;
  }

  if (fDetectorConstruction) {
    const auto config = fDetectorConstruction->GetConfig();
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle(config.sourceParticle);
    if (particle) {
      fParticleSource->GetCurrentSource()->SetParticleDefinition(particle);
    }
    fParticleSource->GetCurrentSource()->GetEneDist()->SetEnergyDisType("Mono");
    fParticleSource->GetCurrentSource()->GetEneDist()->SetMonoEnergy(config.sourceEnergy);
    fParticleSource->GetCurrentSource()->GetPosDist()->SetPosDisType("Point");
    fParticleSource->GetCurrentSource()->GetPosDist()->SetCentreCoords(G4ThreeVector(0., 0., -0.75 * m));
    fParticleSource->GetCurrentSource()->GetAngDist()->SetAngDistType("iso");
    fParticleSource->GetCurrentSource()->GetAngDist()->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  }

  fParticleSource->GeneratePrimaryVertex(anEvent);
}

}  // namespace SourceLab
