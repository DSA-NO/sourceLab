#include "SourceLabActionInitialization.hh"

#include "SourceLabDetectorConstruction.hh"
#include "SourceLabPrimaryGeneratorAction.hh"
#include "SourceLabRunAction.hh"
#include "SourceLabEventAction.hh"

namespace SourceLab
{

SourceLabActionInitialization::SourceLabActionInitialization(
  SourceLabDetectorConstruction* detectorConstruction,
  const G4String& emModel,
  G4bool enableRadioactiveDecay)
: fDetectorConstruction(detectorConstruction)
, fEmModel(emModel)
, fEnableRadioactiveDecay(enableRadioactiveDecay)
{
}

void SourceLabActionInitialization::BuildForMaster() const
{
  SetUserAction(new SourceLabRunAction(fDetectorConstruction, fEmModel, fEnableRadioactiveDecay));
}

void SourceLabActionInitialization::Build() const
{
  SetUserAction(new SourceLabPrimaryGeneratorAction());
  auto* runAction = new SourceLabRunAction(fDetectorConstruction, fEmModel, fEnableRadioactiveDecay);
  SetUserAction(runAction);
  SetUserAction(new SourceLabEventAction(runAction));
}

}  // namespace SourceLab
