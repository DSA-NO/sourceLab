#include "SourceLabDetectorMessenger.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIdirectory.hh"

namespace SourceLab
{
SourceLabDetectorMessenger::SourceLabDetectorMessenger(SourceLabDetectorConstruction* detector)
: fDetector(detector)
{
  auto* geomDir = new G4UIdirectory("/sourceLab/geometry/");
  geomDir->SetGuidance("Generic geometry controls");

  auto* rootDir = new G4UIdirectory("/sourceLab/");
  rootDir->SetGuidance("sourceLab controls");

  fSetWorldSizeCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/worldSize", this);
  fSetWorldSizeCmd->SetGuidance("Set the cubic world side length");
  fSetWorldSizeCmd->SetParameterName("size", false);
  fSetWorldSizeCmd->SetDefaultUnit("cm");

  fSetPhantomRadiusCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/phantomRadius", this);
  fSetPhantomRadiusCmd->SetGuidance("Set the water phantom radius");
  fSetPhantomRadiusCmd->SetParameterName("radius", false);
  fSetPhantomRadiusCmd->SetDefaultUnit("cm");

  fSetPhantomHeightCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/phantomHeight", this);
  fSetPhantomHeightCmd->SetGuidance("Set the water phantom height");
  fSetPhantomHeightCmd->SetParameterName("height", false);
  fSetPhantomHeightCmd->SetDefaultUnit("cm");

  fSetSourceRadiusCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sourceRadius", this);
  fSetSourceRadiusCmd->SetGuidance("Set the Cs-137 source radius");
  fSetSourceRadiusCmd->SetParameterName("radius", false);
  fSetSourceRadiusCmd->SetDefaultUnit("mm");

  fSetSourceShellThicknessCmd =
    new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sourceShellThickness", this);
  fSetSourceShellThicknessCmd->SetGuidance("Set the radial stainless-steel source shell thickness");
  fSetSourceShellThicknessCmd->SetParameterName("thickness", false);
  fSetSourceShellThicknessCmd->SetDefaultUnit("mm");

  fUpdateCmd = new G4UIcommand("/sourceLab/update", this);
  fUpdateCmd->SetGuidance("Update geometry values. Apply this before /run/initialize.");

  fPrintCmd = new G4UIcommand("/sourceLab/print", this);
  fPrintCmd->SetGuidance("Print the current generic detector configuration.");
}

SourceLabDetectorMessenger::~SourceLabDetectorMessenger()
{
  delete fSetWorldSizeCmd;
  delete fSetPhantomRadiusCmd;
  delete fSetPhantomHeightCmd;
  delete fSetSourceRadiusCmd;
  delete fSetSourceShellThicknessCmd;
  delete fUpdateCmd;
  delete fPrintCmd;
}

void SourceLabDetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fSetWorldSizeCmd) {
    fDetector->SetWorldSize(fSetWorldSizeCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetPhantomRadiusCmd) {
    fDetector->SetPhantomRadius(fSetPhantomRadiusCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetPhantomHeightCmd) {
    fDetector->SetPhantomHeight(fSetPhantomHeightCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSourceRadiusCmd) {
    fDetector->SetSourceRadius(fSetSourceRadiusCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSourceShellThicknessCmd) {
    fDetector->SetSourceShellThickness(fSetSourceShellThicknessCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fUpdateCmd) {
    G4cout << "sourceLab configuration updated; apply before /run/initialize." << G4endl;
  }
  else if (command == fPrintCmd || command->GetCommandPath() == "/sourceLab/print") {
    fDetector->PrintConfig();
  }
}

G4String SourceLabDetectorMessenger::GetCurrentValue(G4UIcommand* command)
{
  if (command == fSetWorldSizeCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetWorldSize(), "cm");
  }
  const auto config = fDetector->GetConfig();
  if (command == fSetPhantomRadiusCmd) {
    return G4UIcommand::ConvertToString(config.phantomRadius, "cm");
  }
  if (command == fSetPhantomHeightCmd) {
    return G4UIcommand::ConvertToString(config.phantomHeight, "cm");
  }
  if (command == fSetSourceRadiusCmd) {
    return G4UIcommand::ConvertToString(config.sourceRadius, "mm");
  }
  if (command == fSetSourceShellThicknessCmd) {
    return G4UIcommand::ConvertToString(config.sourceShellThickness, "mm");
  }
  return "";
}

}  // namespace SourceLab
