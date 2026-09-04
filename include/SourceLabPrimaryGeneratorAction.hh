#ifndef SourceLabPrimaryGeneratorAction_h
#define SourceLabPrimaryGeneratorAction_h 1

#include "G4GeneralParticleSource.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

namespace SourceLab
{
class SourceLabDetectorConstruction;

class SourceLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    explicit SourceLabPrimaryGeneratorAction(SourceLabDetectorConstruction* detectorConstruction = nullptr);
    ~SourceLabPrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* anEvent) override;

  private:
    SourceLabDetectorConstruction* fDetectorConstruction = nullptr;
    G4GeneralParticleSource* fParticleSource = nullptr;
};

}  // namespace SourceLab

#endif
