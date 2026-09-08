#include "SourceLabActionInitialization.hh"
#include "SourceLabDetectorConstruction.hh"

#include "FTFP_BERT.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4EmPenelopePhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include <array>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
void PrintUsage()
{
  G4cout << " Usage: " << G4endl;
  G4cout << " sourceLab [-b macro] [-v macro] [-t nThreads] [-p emModel] [-r on|off]" << G4endl;
  G4cout << "   -b macro  : batch mode, execute the given macro" << G4endl;
  G4cout << "   -v macro  : visualize, execute the macro, and keep the UI open" << G4endl;
  G4cout << "   -t N      : number of threads for multi-threaded builds" << G4endl;
  G4cout << "   -p model  : EM model: option4 (default), livermore, penelope" << G4endl;
  G4cout << "   -r mode   : radioactive decay physics: off (default), on" << G4endl;
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
    const auto status = uiManager->ApplyCommand("/control/execute " + candidate);
    if (status == 0) {
      return true;
    }
    G4cerr << "Error: macro '" << candidate << "' failed with UI status " << status << "." << G4endl;
    return false;
  }

  G4cerr << "Error: could not locate macro '" << macro
         << "' in current directory, macros/, or build/." << G4endl;
  return false;
}

struct RuntimeMacroConfig
{
  G4String batchMacroArg;
  G4String visMacroArg;
  G4String workingDirectory;
};

bool IsExistingDirectory(const std::filesystem::path& path)
{
  std::error_code ec;
  return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

std::filesystem::path ResolveExecutableDir(char** argv)
{
  if (argv && argv[0] && *argv[0]) {
    std::error_code ec;
    const auto exePath = std::filesystem::weakly_canonical(argv[0], ec);
    if (!ec) {
      return exePath.parent_path();
    }
  }
  return {};
}

RuntimeMacroConfig ResolveRuntimeMacroConfig(char** argv,
                                             const G4String& macro,
                                             const G4String& visMacro)
{
  RuntimeMacroConfig config;

  auto macroArgForExecution = [](const G4String& macroArg) {
    if (macroArg.empty()) {
      return macroArg;
    }
    const std::filesystem::path path{std::string(macroArg)};
    if (path.has_parent_path()) {
      return G4String(path.filename().string());
    }
    return macroArg;
  };

  auto macroParentIfAny = [](const G4String& macroArg) {
    if (macroArg.empty()) {
      return std::filesystem::path{};
    }
    const std::filesystem::path path{std::string(macroArg)};
    if (!path.has_parent_path()) {
      return std::filesystem::path{};
    }
    return std::filesystem::absolute(path).parent_path();
  };

  config.batchMacroArg = macroArgForExecution(macro);
  config.visMacroArg = macroArgForExecution(visMacro);

  const auto current = std::filesystem::current_path();
  const auto exeDir = ResolveExecutableDir(argv);
  const auto batchMacroParent = macroParentIfAny(macro);
  if (!batchMacroParent.empty() && IsExistingDirectory(batchMacroParent)) {
    config.workingDirectory = G4String(batchMacroParent.string());
    return config;
  }

  const auto visMacroParent = macroParentIfAny(visMacro);
  if (!visMacroParent.empty() && IsExistingDirectory(visMacroParent)) {
    config.workingDirectory = G4String(visMacroParent.string());
    return config;
  }

  if (!exeDir.empty() && IsExistingDirectory(exeDir) && std::filesystem::exists(exeDir / "init_vis.mac")) {
    config.workingDirectory = G4String(exeDir.string());
    return config;
  }

  if (IsExistingDirectory(current / "macros") || std::filesystem::exists(current / "init_vis.mac")) {
    config.workingDirectory = G4String(current.string());
    return config;
  }

  const std::filesystem::path sourceMacros = std::string(SOURCELAB_SOURCE_DIR) + "/macros";
  if (IsExistingDirectory(sourceMacros)) {
    config.workingDirectory = G4String(sourceMacros.string());
    return config;
  }

  config.workingDirectory = G4String(current.string());
  return config;
}

G4bool ApplyWorkingDirectory(const RuntimeMacroConfig& config, G4String& warning)
{
  std::error_code ec;
  std::filesystem::current_path(std::string(config.workingDirectory), ec);
  if (ec) {
    warning = "failed to switch working directory to '" + config.workingDirectory
              + "' for macro resolution.";
    return false;
  }
  warning = "";
  return true;
}

FTFP_BERT* CreatePhysicsList(const G4String& emModel, G4bool enableRadioactiveDecay)
{
  auto* physicsList = new FTFP_BERT;
  if (emModel == "option4") {
    physicsList->ReplacePhysics(new G4EmStandardPhysics_option4());
  }
  else if (emModel == "livermore") {
    physicsList->ReplacePhysics(new G4EmLivermorePhysics());
  }
  else if (emModel == "penelope") {
    physicsList->ReplacePhysics(new G4EmPenelopePhysics());
  }
  else {
    G4cerr << "Error: unknown EM model '" << emModel
           << "'. Use: option4, livermore, penelope" << G4endl;
    return nullptr;
  }

  if (enableRadioactiveDecay) {
    physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());
  }

  return physicsList;
}
}  // namespace

int main(int argc, char** argv)
{
  G4String macro;
  G4String visMacro;
  G4String emModel = "option4";
  G4bool enableRadioactiveDecay = false;

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
    else if (arg == "-p") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      emModel = argv[++i];
    }
    else if (arg == "-r") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      const G4String decayMode = argv[++i];
      if (decayMode == "on") {
        enableRadioactiveDecay = true;
      }
      else if (decayMode == "off") {
        enableRadioactiveDecay = false;
      }
      else {
        G4cerr << "Error: unknown radioactive decay mode '" << decayMode
               << "'. Use: on, off" << G4endl;
        PrintUsage();
        return 1;
      }
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
  auto* physicsList = CreatePhysicsList(emModel, enableRadioactiveDecay);
  if (!physicsList) {
    delete runManager;
    return 1;
  }
  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new SourceLab::SourceLabActionInitialization(detector, emModel, enableRadioactiveDecay));

  auto visManager = new G4VisExecutive;
  visManager->Initialize();

  auto* UImanager = G4UImanager::GetUIpointer();
  const auto runtimeMacroConfig = ResolveRuntimeMacroConfig(argv, macro, visMacro);
  {
    G4String warning;
    if (!ApplyWorkingDirectory(runtimeMacroConfig, warning)) {
      G4cerr << "Warning: " << warning << G4endl;
    }
  }

  if (!macro.empty()) {
    if (!ExecuteMacroWithFallback(UImanager, runtimeMacroConfig.batchMacroArg)) {
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
    if (!ExecuteMacroWithFallback(UImanager, runtimeMacroConfig.visMacroArg)) {
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
