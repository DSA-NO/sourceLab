#include "SourceLabDetectorMessenger.hh"

#include "SourceLabDetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
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
  fSetWorldSizeCmd->SetGuidance("Set the world size");
  fSetWorldSizeCmd->SetParameterName("size", false);
  fSetWorldSizeCmd->SetDefaultUnit("m");

  fSetPhantomHalfXCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/phantomHalfX", this);
  fSetPhantomHalfXCmd->SetGuidance("Set half-size of the phantom in x");
  fSetPhantomHalfXCmd->SetParameterName("x", false);
  fSetPhantomHalfXCmd->SetDefaultUnit("m");

  fSetPhantomHalfYCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/phantomHalfY", this);
  fSetPhantomHalfYCmd->SetGuidance("Set half-size of the phantom in y");
  fSetPhantomHalfYCmd->SetParameterName("y", false);
  fSetPhantomHalfYCmd->SetDefaultUnit("m");

  fSetPhantomHalfZCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/phantomHalfZ", this);
  fSetPhantomHalfZCmd->SetGuidance("Set half-size of the phantom in z");
  fSetPhantomHalfZCmd->SetParameterName("z", false);
  fSetPhantomHalfZCmd->SetDefaultUnit("m");

  fSetSampleDepthCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleDepth", this);
  fSetSampleDepthCmd->SetGuidance("Set the sample depth in the phantom");
  fSetSampleDepthCmd->SetParameterName("depth", false);
  fSetSampleDepthCmd->SetDefaultUnit("cm");

  fSetSampleRadiusCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleRadius", this);
  fSetSampleRadiusCmd->SetGuidance("Set the sample radius");
  fSetSampleRadiusCmd->SetParameterName("radius", false);
  fSetSampleRadiusCmd->SetDefaultUnit("cm");

  fSetSampleThicknessCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleThickness", this);
  fSetSampleThicknessCmd->SetGuidance("Set the sample thickness");
  fSetSampleThicknessCmd->SetParameterName("thickness", false);
  fSetSampleThicknessCmd->SetDefaultUnit("mm");

  fSetSampleSkinThicknessCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleSkinThickness", this);
  fSetSampleSkinThicknessCmd->SetGuidance("Set the sample skin thickness around the cylindrical sample");
  fSetSampleSkinThicknessCmd->SetParameterName("thickness", false);
  fSetSampleSkinThicknessCmd->SetDefaultUnit("mm");

  fSetSampleEndCapAreaCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleEndCapArea", this);
  fSetSampleEndCapAreaCmd->SetGuidance("Set the target area for the dedicated end-cap scorer");
  fSetSampleEndCapAreaCmd->SetParameterName("area", false);
  fSetSampleEndCapAreaCmd->SetDefaultUnit("cm2");

  fSetSampleEndCapThicknessCmd = new G4UIcmdWithADoubleAndUnit("/sourceLab/geometry/sampleEndCapThickness", this);
  fSetSampleEndCapThicknessCmd->SetGuidance("Set the dedicated end-cap thickness");
  fSetSampleEndCapThicknessCmd->SetParameterName("thickness", false);
  fSetSampleEndCapThicknessCmd->SetDefaultUnit("mm");

  fSetSampleAxisCmd = new G4UIcmdWithAString("/sourceLab/geometry/sampleAxis", this);
  fSetSampleAxisCmd->SetGuidance("Set sample axis orientation: z, x, or y");
  fSetSampleAxisCmd->SetParameterName("axis", false);

  fUpdateCmd = new G4UIcommand("/sourceLab/update", this);
  fUpdateCmd->SetGuidance("Update geometry values. Apply this before /run/initialize.");

  fPrintCmd = new G4UIcommand("/sourceLab/print", this);
  fPrintCmd->SetGuidance("Print the current generic detector configuration.");
}

SourceLabDetectorMessenger::~SourceLabDetectorMessenger()
{
  delete fSetWorldSizeCmd;
  delete fSetPhantomHalfXCmd;
  delete fSetPhantomHalfYCmd;
  delete fSetPhantomHalfZCmd;
  delete fSetSampleDepthCmd;
  delete fSetSampleRadiusCmd;
  delete fSetSampleThicknessCmd;
  delete fSetSampleSkinThicknessCmd;
  delete fSetSampleEndCapAreaCmd;
  delete fSetSampleEndCapThicknessCmd;
  delete fSetSampleAxisCmd;
  delete fUpdateCmd;
  delete fPrintCmd;
}

void SourceLabDetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fSetWorldSizeCmd) {
    fDetector->SetWorldSize(fSetWorldSizeCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetPhantomHalfXCmd) {
    fDetector->SetPhantomHalfSize(fSetPhantomHalfXCmd->GetNewDoubleValue(newValue),
      fDetector->GetWorldSize(),
      fDetector->GetSampleDepth());
  }
  else if (command == fSetPhantomHalfYCmd) {
    fDetector->SetPhantomHalfSize(fDetector->GetWorldSize(),
      fSetPhantomHalfYCmd->GetNewDoubleValue(newValue),
      fDetector->GetSampleDepth());
  }
  else if (command == fSetPhantomHalfZCmd) {
    fDetector->SetPhantomHalfSize(fDetector->GetWorldSize(),
      fDetector->GetSampleDepth(),
      fSetPhantomHalfZCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleDepthCmd) {
    fDetector->SetSampleDepth(fSetSampleDepthCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleRadiusCmd) {
    fDetector->SetSampleSize(fSetSampleRadiusCmd->GetNewDoubleValue(newValue),
      fDetector->GetSampleThickness());
  }
  else if (command == fSetSampleThicknessCmd) {
    fDetector->SetSampleSize(fDetector->GetSampleRadius(),
      fSetSampleThicknessCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleSkinThicknessCmd) {
    fDetector->SetSampleSkinThickness(fSetSampleSkinThicknessCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleEndCapAreaCmd) {
    fDetector->SetSampleEndCapArea(fSetSampleEndCapAreaCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleEndCapThicknessCmd) {
    fDetector->SetSampleEndCapThickness(fSetSampleEndCapThicknessCmd->GetNewDoubleValue(newValue));
  }
  else if (command == fSetSampleAxisCmd) {
    fDetector->SetSampleAxis(newValue);
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
    return G4UIcommand::ConvertToString(fDetector->GetWorldSize(), "m");
  }
  if (command == fSetPhantomHalfXCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetConfig().phantomHalfX, "m");
  }
  if (command == fSetPhantomHalfYCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetConfig().phantomHalfY, "m");
  }
  if (command == fSetPhantomHalfZCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetConfig().phantomHalfZ, "m");
  }
  if (command == fSetSampleDepthCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleDepth(), "cm");
  }
  if (command == fSetSampleRadiusCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleRadius(), "cm");
  }
  if (command == fSetSampleThicknessCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleThickness(), "mm");
  }
  if (command == fSetSampleSkinThicknessCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleSkinThickness(), "mm");
  }
  if (command == fSetSampleEndCapAreaCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleEndCapArea(), "cm2");
  }
  if (command == fSetSampleEndCapThicknessCmd) {
    return G4UIcommand::ConvertToString(fDetector->GetSampleEndCapThickness(), "mm");
  }
  if (command == fSetSampleAxisCmd) {
    return fDetector->GetSampleAxis();
  }
  return "";
}

}  // namespace SourceLab
