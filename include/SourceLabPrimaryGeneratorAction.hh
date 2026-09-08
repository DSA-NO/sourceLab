#ifndef SourceLabPrimaryGeneratorAction_h
#define SourceLabPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

class G4Event;
class G4GeneralParticleSource;

namespace SourceLab
{
class SourceLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    SourceLabPrimaryGeneratorAction();
    ~SourceLabPrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

  private:
    G4GeneralParticleSource* fParticleSource = nullptr;
};

}  // namespace SourceLab

#endif
