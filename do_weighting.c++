#include "TFile.h"
#include "TTree.h"
#include "EDepSim/TG4Event.h"
#include "EDepSim/TG4Trajectory.h"

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"

#include "cetlib/filepath_maker.h"
#include "cetlib/filesystem.h"
#include "cetlib/search_path.h"
#include "fhiclcpp/intermediate_table.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/ParameterSet.h"

#include "geant4reweight/src/ReweightBase/G4ReweightManager.hh"
#include "geant4reweight/src/ReweightBase/G4MultiReweighter.hh"
#include "geant4reweight/src/ReweightBase/G4ReweightTraj.hh"
#include "geant4reweight/src/ReweightBase/G4ReweightStep.hh"

fhicl::ParameterSet MakeFCLPars(std::string fcl_file) {
  char const* fhicl_env = getenv("FHICL_FILE_PATH");
  std::string search_path;
  if (fhicl_env == nullptr) {
    std::cerr << "Expected environment variable FHICL_FILE_PATH is missing or empty: using \".\"\n";
    search_path = ".";
  }
  else {
    search_path = std::string{fhicl_env};
  }

  cet::filepath_first_absolute_or_lookup_with_dot lookupPolicy{search_path};
  return fhicl::ParameterSet::make(fcl_file, lookupPolicy);
};

TFile * OpenFile(const std::string filename) {
  TFile * theFile = 0x0;
  std::cout << "Searching for " << filename << std::endl;
  if (cet::file_exists(filename)) {
    std::cout << "File exists. Opening " << filename << std::endl;
    theFile = new TFile(filename.c_str());
    if (!theFile ||theFile->IsZombie() || !theFile->IsOpen()) {
      delete theFile;
      theFile = 0x0;
      throw cet::exception("PDSPAnalyzer_module.cc") << "Could not open " << filename;
    }
  }
  else {
    std::cout << "File does not exist here. Searching FW_SEARCH_PATH" << std::endl;
    cet::search_path sp{"FW_SEARCH_PATH"};
    std::string found_filename;
    auto found = sp.find_file(filename, found_filename);
    if (!found) {
      throw cet::exception("PDSPAnalyzer_module.cc") << "Could not find " << filename;
    }

    std::cout << "Found file " << found_filename << std::endl;
    theFile = new TFile(found_filename.c_str());
    if (!theFile ||theFile->IsZombie() || !theFile->IsOpen()) {
      delete theFile;
      theFile = 0x0;
      throw cet::exception("PDSPAnalyzer_module.cc") << "Could not open " << found_filename;
    }
  }
  return theFile;
};
class EDepTraj {
 public:
  EDepTraj(const TG4Trajectory & traj) {
    fParentID = traj.GetParentId();
    fPDG = traj.GetPDGCode();
    fID = traj.GetTrackId();
  }
  int fParentID, fPDG, fID;
};

std::vector<int> ValidPDGs {
  211, -211, 111,
  2212, 2112,
  321, -321
};

class EDepSimEvent {
 public:
  EDepSimEvent(TG4Event * event) {
    std::map<int, std::vector<EDepTraj>> children;
    for (size_t i = 0; i < event->Trajectories.size(); ++i) {

      const auto & traj = event->Trajectories[i];
      EDepTraj edep_traj(traj);
      if (edep_traj.fParentID == -1) {
        fPrimaryPDG = edep_traj.fPDG;
        fPrimaryID = edep_traj.fID;
        fPrimaryTraj = &traj;
      }
      if (std::find(ValidPDGs.begin(), ValidPDGs.end(), edep_traj.fPDG)
          != ValidPDGs.end()) {
        children[edep_traj.fID].push_back(edep_traj);
      }
    }
    fPrimaryChildren = children[fPrimaryID];
  };

  bool IsInteraction() {
    return (fPrimaryTraj->Points.back().GetProcess() == 4 &&
            fPrimaryTraj->Points.back().GetSubprocess() == 121);
  };

  int fPrimaryPDG, fPrimaryID = -999;
  const TG4Trajectory * fPrimaryTraj = 0x0;
  std::vector<EDepTraj> fPrimaryChildren;
};

int main(int argc, char ** argv) {
  namespace po = boost::program_options;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")
      ("i", po::value<std::string>(), "Input file")
      ("c", po::value<std::string>(), "Fcl file")
      ("o", po::value<std::string>(), "Output file");
  
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    
  
  if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
  }

  std::string input_file = "";
  if (vm.count("i")) {
    input_file = vm["i"].as<std::string>();
  }
  else {
    std::cout << "Need file" << std::endl;
    return 1;
  }

  std::string output_file = "";
  if (vm.count("o")) {
    output_file = vm["o"].as<std::string>();
  }
  else {
    std::cout << "Need output file" << std::endl;
    return 1;
  }

  std::string fcl_file = "";
  if (vm.count("c")) {
    fcl_file = vm["c"].as<std::string>();
  }
  else {
    std::cout << "Need fcl file" << std::endl;
    return 1;
  }

  //
  //Get Parameters
  auto fcl_pars = MakeFCLPars(fcl_file);
  auto material = fcl_pars.get<fhicl::ParameterSet>("Material");
  auto fracs_file_name = fcl_pars.get<std::string>("FracsFile");
  auto g4rw_pars = fcl_pars.get<std::vector<fhicl::ParameterSet>>("ParameterSet");
  auto set_weight = fcl_pars.get<double>("Weight", 2.);

  //Open the file holding the final state fractions
  TFile * fracs_file = OpenFile(fracs_file_name);
  std::cout << "Fracs file: " << fracs_file << std::endl;

  //Set up reweighting objects
  G4ReweightManager * RWManager = new G4ReweightManager({material});
  G4MultiReweighter * reweighter = new G4MultiReweighter(
    211, *fracs_file, g4rw_pars, material, RWManager);
  reweighter->SetAllParameterValues({set_weight}); //Set the weight

  //Inputfile 
  TFile f(input_file.c_str(), "open");
  TTree * tree = (TTree*)f.Get("EDepSimEvents");
  TG4Event * event = 0x0;
  tree->SetBranchAddress("Event", &event);

  int ninteractions = 0;
  double nweighted = 0., ntotal = 0.;

  //Output File
  TFile fOut(output_file.c_str(), "recreate");
  TTree outtree("tree", "");
  double weight = 1.;
  double out_len = 0.;
  outtree.Branch("weight", &weight);
  outtree.Branch("len", &out_len);

  //Loop over edep sim events
  for (int i = 0; i < tree->GetEntries(); ++i) {
    std::cout << "############" << std::endl;
    tree->GetEntry(i);
    EDepSimEvent edep_event(event);

    if (edep_event.IsInteraction()) ++ninteractions;

    //Start building reweightable trajectory
    G4ReweightTraj primary_traj(
        edep_event.fPrimaryID,
        edep_event.fPrimaryPDG,
        -1, i, {0,0});
    for (auto & child_traj : edep_event.fPrimaryChildren) {
      primary_traj.AddChild(
        new G4ReweightTraj(child_traj.fID, child_traj.fPDG,
                           edep_event.fPrimaryID, i, {0, 0})
      );
    }

    //Build the path it took
    const auto & pt0 = edep_event.fPrimaryTraj->Points[0];
    std::cout << 0 << " " << pt0.GetPosition().X()/10. << " " <<
                 pt0.GetPosition().Y()/10. << " " << pt0.GetPosition().Z()/10. <<
                 " " << pt0.GetProcess() << " " << pt0.GetSubprocess() <<
                 " " << sqrt(pt0.GetMomentum()[0]*pt0.GetMomentum()[0] +
                             pt0.GetMomentum()[1]*pt0.GetMomentum()[1] +
                             pt0.GetMomentum()[2]*pt0.GetMomentum()[2]) <<
                 std::endl;
    out_len = 0.;
    for (size_t j = 1; j < edep_event.fPrimaryTraj->Points.size(); ++j) {
      std::string proc = "default";
      if ((j == edep_event.fPrimaryTraj->Points.size()-1) &&
          edep_event.IsInteraction()) {//Last step
        proc = "pi+Inelastic";
      }
      const auto & pt = edep_event.fPrimaryTraj->Points[j];
      const auto & prev_pt = edep_event.fPrimaryTraj->Points[j-1];
      auto dist = (pt.GetPosition() - prev_pt.GetPosition());
      double len = sqrt(dist.X()*dist.X() + dist.Y()*dist.Y() +
                        dist.Z()*dist.Z())/10.;
      out_len += len;
      double preStepP[3] = {
        prev_pt.GetMomentum()[0],
        prev_pt.GetMomentum()[1],
        prev_pt.GetMomentum()[2]
      };
      double postStepP[3] = {
        pt.GetMomentum()[0],
        pt.GetMomentum()[1],
        pt.GetMomentum()[2]
      };

      double total_pre_p = sqrt(preStepP[0]*preStepP[0] +
                                preStepP[1]*preStepP[1] +
                                preStepP[2]*preStepP[2]);

      double total_post_p = sqrt(postStepP[0]*postStepP[0] +
                                postStepP[1]*postStepP[1] +
                                postStepP[2]*postStepP[2]);

      std::cout << j << " " << pt.GetPosition().X()/10. << " " <<
                   pt.GetPosition().Y()/10. << " " <<
                   pt.GetPosition().Z()/10. << " " << pt.GetProcess() <<
                   " " << pt.GetSubprocess() << " " <<
                   len <<
                   " " << total_pre_p << " " << total_post_p << std::endl;

      G4ReweightStep * step = new G4ReweightStep(edep_event.fPrimaryID,
                                                 edep_event.fPrimaryPDG,
                                                 0, i, preStepP, postStepP,
                                                 len, proc);
      primary_traj.AddStep(step);
    }

    //Get the weight for this trajectory/event
    weight = reweighter->GetWeightFromSetParameters(primary_traj);
    std::cout << "Weight: " << weight << std::endl;;
    if (edep_event.IsInteraction()) nweighted += weight;
    ntotal += weight;


    outtree.Fill();
  }

  std::cout << std::endl;
  std::cout << "Interaction rate: " << 100.*ninteractions/tree->GetEntries() << std::endl;
  std::cout << "Weighted rate: " << 100.*nweighted/tree->GetEntries() << std::endl;
  std::cout << "Total weights: " << ntotal << std::endl;

  outtree.Write();
  fOut.Close();

  f.Close();

  return 0;
}
