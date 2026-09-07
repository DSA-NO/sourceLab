#include "SourceLabPrimaryGeneratorAction.hh"

#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

namespace SourceLab
{

SourceLabPrimaryGeneratorAction::SourceLabPrimaryGeneratorAction()
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

  fParticleSource->GeneratePrimaryVertex(anEvent);
}

}  // namespace SourceLab
