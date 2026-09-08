#include "SourceLabPrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"

namespace SourceLab
{

SourceLabPrimaryGeneratorAction::SourceLabPrimaryGeneratorAction()
{
  // GPS provides particle generation configured entirely through macros or UI commands.
  fParticleSource = new G4GeneralParticleSource();
}

SourceLabPrimaryGeneratorAction::~SourceLabPrimaryGeneratorAction()
{
  delete fParticleSource;
}

void SourceLabPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  fParticleSource->GeneratePrimaryVertex(event);
}

}  // namespace SourceLab
