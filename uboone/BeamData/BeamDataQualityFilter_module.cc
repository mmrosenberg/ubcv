////////////////////////////////////////////////////////////////////////
// Class:       BeamDataQualityFilter
// Module Type: filter
// File:        BeamDataQualityFilter_module.cc
//
// Generated at Tue May 10 13:33:22 2016 by Zarko Pavlovic using artmod
// from cetpkgsupport v1_10_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RawData/BeamInfo.h"
#include "lardataobj/RawData/TriggerData.h"

#include <memory>
#include "getFOM.h"

class BeamDataQualityFilter;

class BeamDataQualityFilter : public art::EDFilter {
public:
  explicit BeamDataQualityFilter(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  BeamDataQualityFilter(BeamDataQualityFilter const &) = delete;
  BeamDataQualityFilter(BeamDataQualityFilter &&) = delete;
  BeamDataQualityFilter & operator = (BeamDataQualityFilter const &) = delete;
  BeamDataQualityFilter & operator = (BeamDataQualityFilter &&) = delete;

  // Required functions.
  bool filter(art::Event & e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;

private:

  // Declare member data here.
  typedef struct {
    std::string fBeamName;
    bool fRecalculateFOM;
    std::vector<double> fHornCurrentRange;
    std::vector<double> fIntensityRange;
    std::vector<double> fFOMRange;

    int fNHornCurrentCut;
    int fNIntensityCut;
    int fNFOMCut;
  } beamcuts_t;
  std::map<uint32_t, beamcuts_t> fBeamCutMap;

};


BeamDataQualityFilter::BeamDataQualityFilter(fhicl::ParameterSet const & p)
// :
// Initialize member data here.
{
  // Call appropriate produces<>() functions here.
  std::vector<std::string> event_types=p.get<std::vector<std::string> >("apply_to_events");

  for (auto et : event_types) {
    fhicl::ParameterSet eps=p.get<fhicl::ParameterSet>(et);
    beamcuts_t bc;
    uint32_t trigger_mask=eps.get<uint32_t>("trigger_mask");    
    bc.fRecalculateFOM=eps.get<bool>("recalculate_fom");
    bc.fBeamName=eps.get<std::string>("beam_name");
    bc.fHornCurrentRange=eps.get<std::vector<double> >("horn_current_range");
    bc.fIntensityRange=eps.get<std::vector<double> >("intensity_range");
    bc.fFOMRange=eps.get<std::vector<double> >("fom_range");
    std::pair<uint32_t, beamcuts_t> bcpair(trigger_mask, bc);
    fBeamCutMap.insert(bcpair);
  }
}

bool BeamDataQualityFilter::filter(art::Event & e)
{
   // Implementation of required member function here.
  bool flt=true;
  art::Handle< std::vector<raw::Trigger> > triggerHandle;
  std::vector<art::Ptr<raw::Trigger> > trigInfo;
  if (e.getByLabel("daq", triggerHandle))
    art::fill_ptr_vector(trigInfo, triggerHandle);
  else {
    mf::LogWarning(__FUNCTION__) << "Missing trigger info. Skipping event.";
    return flt;
  }
  std::bitset<16> trigbit(trigInfo[0]->TriggerBits());
  if (fBeamCutMap.find(trigInfo[0]->TriggerBits())==fBeamCutMap.end()) {
    mf::LogInfo(__FUNCTION__) << "Trigger not matching any of the beam(s). Skipping beam quality filter. trigger bits= "<<trigInfo[0]->TriggerBits()<<" "<<trigbit;
    return flt;
  }
  beamcuts_t* bc=&fBeamCutMap[trigInfo[0]->TriggerBits()];

  art::Handle< raw::BeamInfo > beam;
  if (e.getByLabel("beamdata",beam)){
    std::map<std::string, std::vector<double>> datamap = beam->GetDataMap();
    float fom=-99999;
    if (bc->fRecalculateFOM) {
      fom=1;//getFOM();
    } else {
      //get it from BeamInfo
      if (datamap["FOM"].size()>0)
	fom=datamap["FOM"][0];
      else
	mf::LogError(__FUNCTION__)<<"recalculate_fom set to false, but FOM is not in beamdata product";
    }
    if ( fom<bc->fFOMRange[0] || fom >bc->fFOMRange[1]) {
      flt=false;
      bc->fNFOMCut+=1;
    }
    float intensity=0;
    if (bc->fBeamName=="bnb" && datamap["E:TOR860"].size()>0) intensity=datamap["E:TOR860"][0];
    else if (bc->fBeamName=="bnb" && datamap["E:TOR875"].size()>0) intensity=datamap["E:TOR875"][0];
    else if (bc->fBeamName=="numi" && datamap["E:TORTGT"].size()>0) intensity=datamap["E:TORTGT"][0];
    if ( intensity<bc->fIntensityRange[0] || intensity>bc->fIntensityRange[1]) {
      flt=false;
      bc->fNIntensityCut+=1;
    }
    float horncurr=0;
    if (bc->fBeamName=="bnb" && datamap["E:THCURR"].size()>0) horncurr=datamap["E:THCURR"][0];
    else if (bc->fBeamName=="numi" && datamap["E:NSLINA"].size()>0 &&
	     datamap["E:NSLINB"].size()>0 && datamap["E:NSLINC"].size()>0 &&
	     datamap["E:NSLIND"].size()>0) 
      horncurr=-datamap["E:NSLINA"][0]-datamap["E:NSLINB"][0]-datamap["E:NSLINC"][0]-datamap["E:NSLIND"][0];
    if ( horncurr<bc->fHornCurrentRange[0] || horncurr>bc->fHornCurrentRange[1]) {
      flt=false;
      bc->fNHornCurrentCut+=1;
    }
  } else {
    mf::LogError(__FUNCTION__)<<"Running beam data quality filter, but missing beam data!";
  }
  
  return flt;
}

void BeamDataQualityFilter::beginJob()
{
  // Implementation of optional member function here.
  for (auto& bc: fBeamCutMap) {
    bc.second.fRecalculateFOM=false;
    bc.second.fNHornCurrentCut=0;
    bc.second.fNIntensityCut=0;
    bc.second.fNFOMCut=0;
  }
}

void BeamDataQualityFilter::endJob()
{
  // Implementation of optional member function here.
  std::stringstream ss;
  for (auto& bc: fBeamCutMap) {
    ss<<"Beam name: "<<bc.second.fBeamName<<std::endl;
    ss<<"Events failed due to intensity cut: "<<bc.second.fNIntensityCut<<std::endl;
    ss<<"Events failed due to horn current cut: "<<bc.second.fNHornCurrentCut<<std::endl;
    ss<<"Events failed due to FOM cut: "<<bc.second.fNFOMCut<<std::endl;
  }
  mf::LogInfo(__FUNCTION__)<<ss.str();
}

DEFINE_ART_MODULE(BeamDataQualityFilter)
