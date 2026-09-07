#ifndef SourceLabPrimaryGeneratorAction_h
#define SourceLabPrimaryGeneratorAction_h 1

#include "G4GeneralParticleSource.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

namespace SourceLab
{
class SourceLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    SourceLabPrimaryGeneratorAction();
    ~SourceLabPrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* anEvent) override;

  private:
    G4GeneralParticleSource* fParticleSource = nullptr;
};

}  // namespace SourceLab

#endif
