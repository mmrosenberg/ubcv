////////////////////////////////////////////////////////////////////////
// Class:       LArSoftSuperaSriver
// Module Type: analyzer
// File:        LArSoftSuperaSriver_module.cc
//
// Generated at Tue May  2 17:36:15 2017 by Kazuhiro Terao using artmod
// from cetpkgsupport v1_11_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/MCBase/MCShower.h"
#include "lardataobj/MCBase/MCTrack.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "larevt/CalibrationDBI/Interface/ChannelStatusService.h"
#include "larevt/CalibrationDBI/Interface/ChannelStatusProvider.h"
#include "larcore/Geometry/WireReadout.h"
#include "larcore/CoreUtils/ServiceUtil.h" // lar::providerFrom<>()
#include <TString.h>
#include <TTimeStamp.h>

#include "LArCVMetaMaker.h"
#include "LArCVSuperaDriver.h"
#include "Base/PSet.h"

class LArSoftSuperaSriver;

class LArSoftSuperaSriver : public art::EDAnalyzer {
public:
  explicit LArSoftSuperaSriver(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  LArSoftSuperaSriver(LArSoftSuperaSriver const &) = delete;
  LArSoftSuperaSriver(LArSoftSuperaSriver &&) = delete;
  LArSoftSuperaSriver & operator = (LArSoftSuperaSriver const &) = delete;
  LArSoftSuperaSriver & operator = (LArSoftSuperaSriver &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  void beginJob() override;
  void endJob() override;
private:

  // blank vectors for datasets where handles are invalid, but we wish to continue.
  std::vector<supera::LArMCTrack_t>  _blank_mctrack_v;
  std::vector<supera::LArMCShower_t> _blank_mcshower_v;

  // Declare member data here.
  larcv::LArCVSuperaDriver _supera;
  bool _fAllowInvalidMCReco;
};


LArSoftSuperaSriver::LArSoftSuperaSriver(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
 // More initializers here.
  _fAllowInvalidMCReco(true)
{

  std::string supera_cfg;
  cet::search_path finder("FHICL_FILE_PATH");

  if( !finder.find_file(p.get<std::string>("supera_params"), supera_cfg) )
    throw cet::exception("LArSoftSuperaSriver") << "Unable to find supera cfg in "  << finder.to_string() << "\n";

  _supera.configure(supera_cfg);

  _fAllowInvalidMCReco = p.get<bool>("AllowInvalidMCReco",true);

  auto process_names = _supera.ProcessNames();
  for(auto const& proc_name : process_names) {
    std::string param_name = "CSV" + proc_name;
    auto constraint_file = p.get<std::string>(param_name.c_str(),"");
    if(constraint_file.empty()) continue;

    std::string fullpath;
    if( !finder.find_file(constraint_file,fullpath) )
      throw cet::exception("LArSoftSuperaSriver") << "Unable to find CSV file "  << constraint_file << "\n";

    _supera.SetCSV(proc_name,fullpath);
  }

  // Decide on output filename
  auto out_fname = p.get<std::string>("out_filename","");
  if(p.get<bool>("unique_filename") || out_fname.empty()) {
    TString tmp_fname;
    if(out_fname.empty())
      tmp_fname = "emptyname_" + p.get<std::string>("stream");
    else {
      tmp_fname = out_fname;
      tmp_fname.ReplaceAll(".root","");
      tmp_fname += "_" + p.get<std::string>("stream");
    }
    TTimeStamp ts;
    out_fname = Form("%s_%08d_%06d_%06d.root",tmp_fname.Data(),ts.GetDate(),ts.GetTime(), (int)(ts.GetNanoSec()/1.e3));
  }

  _supera.override_output_file(out_fname);

  art::ServiceHandle<util::LArCVMetaMaker> metamaker;
  metamaker->addJson(out_fname,p.get<std::string>("stream"));
}

void LArSoftSuperaSriver::beginJob()
{
  _supera.initialize();
}

void LArSoftSuperaSriver::analyze(art::Event const & e)
{

  //
  // set data pointers
  //

  // hit
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArHit_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<recob::Hit> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) {
      std::cerr<< "Attempted to load Hit data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate Hit data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // wire
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArWire_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<recob::Wire> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      std::cerr<< "Attempted to load Wire data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate Wire data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // opdigit
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArOpDigit_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<raw::OpDetWaveform> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      std::cerr<< "Attempted to load OpDigit data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate OpDigit data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // mctruth
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArMCTruth_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<simb::MCTruth> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      std::cerr<< "Attempted to load LArMCTruth data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate MCTruth data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // mctrack
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArMCTrack_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<sim::MCTrack> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      if ( !_fAllowInvalidMCReco )
	throw cet::exception("LArSoftSuperaSriver") << "Could not locate MCTrack data!" << std::endl;
      else
	_supera.SetDataPointer( _blank_mctrack_v, label );
    }
    else 
      _supera.SetDataPointer(*data_h,label);
  }

  // mcshower
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArMCShower_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<sim::MCShower> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      if ( !_fAllowInvalidMCReco )
	throw cet::exception("LArSoftSuperaSriver") << "Could not locate MCTrack data!" << std::endl;
      else
	_supera.SetDataPointer( _blank_mcshower_v, label );
    }
    else
      _supera.SetDataPointer(*data_h,label);
  }

  // simch
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArSimCh_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<sim::SimChannel> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      std::cerr<< "Attempted to load SimChannel data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate SimChannel data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // simedep
  for(auto const& label : _supera.DataLabels(::supera::LArDataType_t::kLArSimEdep_t)) {
    if(label.empty()) continue;
    art::Handle<std::vector<sim::SimEnergyDeposit> > data_h;
    if(label.find(" ")<label.size()) {
      e.getByLabel(label.substr(0,label.find(" ")),
		   label.substr(label.find(" ")+1,label.size()-label.find(" ")-1),
		   data_h);
    }else{ e.getByLabel(label, data_h); }
    if(!data_h.isValid()) { 
      std::cerr<< "Attempted to load SimEnergyDeposit data: " << label << std::endl;
      throw cet::exception("LArSoftSuperaSriver") << "Could not locate SimEnergyDeposit data!" << std::endl; 
    }
    _supera.SetDataPointer(*data_h,label);
  }

  // chstatus
  auto supera_chstatus = _supera.SuperaChStatusPointer();
  if(supera_chstatus) {

    // Set database status
    auto const& channelMap = art::ServiceHandle<geo::WireReadout>()->Get();
    const lariov::ChannelStatusProvider& chanFilt = art::ServiceHandle<lariov::ChannelStatusService>()->GetProvider();
    for(size_t i=0; i < channelMap.Nchannels(); ++i) {
      auto const wid = channelMap.ChannelToWire(i).front();
      if (!chanFilt.IsPresent(i)) supera_chstatus->set_chstatus(wid.Plane, wid.Wire, ::larcv::chstatus::kNOTPRESENT);
      else supera_chstatus->set_chstatus(wid.Plane, wid.Wire, (short)(chanFilt.Status(i)));
    }

    /*
    std::vector<bool> filled_ch( ::larcv::supera::Nchannels(), false );
    // If specified check RawDigit pedestal value: if negative this channel is not used by wire (set status=>-2)
    if(!_core.producer_digit().empty()) {
      art::Handle<std::vector<raw::RawDigit> > digit_h;
      e.getByLabel(_core.producer_digit(),digit_h);
      for(auto const& digit : *digit_h) {
        auto const ch = digit.Channel();
        if(ch >= filled_ch.size()) throw ::larcv::larbys("Found RawDigit > possible channel number!");
        if(digit.GetPedestal()<0.) {
          _core.set_chstatus(ch,::larcv::chstatus::kNEGATIVEPEDESTAL);
          filled_ch[ch] = true;
        }
      }
    }

    // Set database status
    const lariov::ChannelStatusProvider& chanFilt = art::ServiceHandle<lariov::ChannelStatusService>()->GetProvider();
    for(size_t i=0; i < ::larcv::supera::Nchannels(); ++i) {
      if ( filled_ch[i] ) continue;
      if (!chanFilt.IsPresent(i)) _core.set_chstatus(i,::larcv::chstatus::kNOTPRESENT);
      else _core.set_chstatus(i,(short)(chanFilt.Status(i)));
    }
    */
  }

  //
  // execute supera
  //
  _supera.process(e.id().run(),e.id().subRun(),e.id().event());
}

void LArSoftSuperaSriver::endJob()
{
  _supera.finalize();
}

DEFINE_ART_MODULE(LArSoftSuperaSriver)
