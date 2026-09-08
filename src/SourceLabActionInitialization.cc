#include "SourceLabActionInitialization.hh"

#include "SourceLabDetectorConstruction.hh"
#include "SourceLabPrimaryGeneratorAction.hh"
#include "SourceLabRunAction.hh"
#include "SourceLabEventAction.hh"

namespace SourceLab
{

SourceLabActionInitialization::SourceLabActionInitialization(
  SourceLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
}

void SourceLabActionInitialization::BuildForMaster() const
{
  SetUserAction(new SourceLabRunAction(fDetectorConstruction));
}

void SourceLabActionInitialization::Build() const
{
  SetUserAction(new SourceLabPrimaryGeneratorAction());
  auto* runAction = new SourceLabRunAction(fDetectorConstruction);
  SetUserAction(runAction);
  SetUserAction(new SourceLabEventAction(runAction));
}

}  // namespace SourceLab
