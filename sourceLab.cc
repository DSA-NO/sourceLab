#include "SourceLabActionInitialization.hh"
#include "SourceLabDetectorConstruction.hh"

#include "FTFP_BERT.hh"
#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include <array>
#include <fstream>

namespace
{
void PrintUsage()
{
  G4cout << " Usage: " << G4endl;
  G4cout << " sourceLab [-b macro] [-v macro] [-t nThreads]" << G4endl;
  G4cout << "   -b macro  : batch mode, execute the given macro" << G4endl;
  G4cout << "   -v macro  : visualize, execute the macro, and keep the UI open" << G4endl;
  G4cout << "   -t N      : number of threads for multi-threaded builds" << G4endl;
  G4cout << "   (no args) : interactive session" << G4endl;
}

bool FileExists(const G4String& path)
{
  std::ifstream f(path);
  return f.good();
}

bool ExecuteMacroWithFallback(G4UImanager* uiManager, const G4String& macro)
{
  const std::array<G4String, 3> candidates = {
    macro,
    "macros/" + macro,
    "build/" + macro,
  };

  for (const auto& candidate : candidates) {
    if (!FileExists(candidate)) {
      continue;
    }
    uiManager->ApplyCommand("/control/execute " + candidate);
    return true;
  }

  G4cerr << "Error: could not locate macro '" << macro
         << "' in current directory, macros/, or build/." << G4endl;
  return false;
}
}  // namespace

int main(int argc, char** argv)
{
  G4String macro;
  G4String visMacro;

#ifdef G4MULTITHREADED
  G4int nThreads = 0;
#endif

  for (G4int i = 1; i < argc; ++i) {
    G4String arg = argv[i];
    if (arg == "-b") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      macro = argv[++i];
    }
    else if (arg == "-v") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      visMacro = argv[++i];
    }
#ifdef G4MULTITHREADED
    else if (arg == "-t") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      nThreads = std::atoi(argv[++i]);
    }
#endif
    else {
      PrintUsage();
      return 1;
    }
  }

  if (!macro.empty() && !visMacro.empty()) {
    G4cerr << "Error: -b and -v are mutually exclusive." << G4endl;
    PrintUsage();
    return 1;
  }

  G4UIExecutive* ui = nullptr;
  if (macro.empty()) {
    ui = new G4UIExecutive(argc, argv);
  }

  auto runManager = G4RunManagerFactory::CreateRunManager();
#ifdef G4MULTITHREADED
  if (nThreads > 0) {
    runManager->SetNumberOfThreads(nThreads);
  }
#endif

  auto detector = new SourceLab::SourceLabDetectorConstruction();
  runManager->SetUserInitialization(detector);
  runManager->SetUserInitialization(new FTFP_BERT);
  runManager->SetUserInitialization(new SourceLab::SourceLabActionInitialization(detector));

  auto visManager = new G4VisExecutive;
  visManager->Initialize();

  auto* UImanager = G4UImanager::GetUIpointer();

  if (!macro.empty()) {
    if (!ExecuteMacroWithFallback(UImanager, macro)) {
      delete visManager;
      delete runManager;
      return 1;
    }
  }
  else if (!visMacro.empty()) {
    if (!ExecuteMacroWithFallback(UImanager, "init_vis.mac")) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    if (ui && ui->IsGUI()) {
      if (!ExecuteMacroWithFallback(UImanager, "gui.mac")) {
        delete ui;
        delete visManager;
        delete runManager;
        return 1;
      }
    }
    if (!ExecuteMacroWithFallback(UImanager, visMacro)) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    ui->SessionStart();
    delete ui;
  }
  else {
    if (!ExecuteMacroWithFallback(UImanager, "init_vis.mac")) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    if (ui && ui->IsGUI()) {
      if (!ExecuteMacroWithFallback(UImanager, "gui.mac")) {
        delete ui;
        delete visManager;
        delete runManager;
        return 1;
      }
    }
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
