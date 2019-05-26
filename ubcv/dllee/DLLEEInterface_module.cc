////////////////////////////////////////////////////////////////////////
// Class:       DLLEEInterface
// Plugin Type: producer (art v2_05_01)
// File:        DLLEEInterface_module.cc
//
// Generated at Wed Aug 29 05:49:09 2018 by Taritree,,, using cetskelgen
// from cetlib version v1_21_00.
////////////////////////////////////////////////////////////////////////

//#include "Python.h"

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>

// opencv
#include "opencv/cv.h"
#include "opencv2/opencv.hpp"

// larsoft
#include "lardataobj/RecoBase/OpFlash.h"

// ublite
#include "ublite/LiteMaker/ScannerAlgo.h"

// ubcv
#include "ubcv/LArCVImageMaker/LArCVSuperaDriver.h"
#include "ubcv/LArCVImageMaker/ImageMetaMaker.h"
#include "ubcv/LArCVImageMaker/SuperaMetaMaker.h"
#include "ubcv/LArCVImageMaker/SuperaWire.h"
#include "ubcv/LArCVImageMaker/LAr2Image.h"
#include "ubcv/ubdldata/pixeldata.h"

// larcv
#include "Base/larcv_base.h"
#include "Base/PSet.h"
#include "Base/LArCVBaseUtilFunc.h"
#include "DataFormat/EventImage2D.h"
// #include "larcv/core/DataFormat/Image2D.h"
// #include "larcv/core/DataFormat/ImageMeta.h"
// #include "larcv/core/DataFormat/ROI.h"
// #include "larcv/core/json/json_utils.h"

// larlite
#include "DataFormat/opflash.h"
#include "SelectionTool/LEEPreCuts/LEEPreCut.h"

// larlitecv
#include "TaggerCROI/TaggerCROIAlgo.h"
#include "TaggerCROI/TaggerCROIAlgoConfig.h"
#include "TaggerCROI/TaggerCROITypes.h"
#include "GapChs/EmptyChannelAlgo.h"

// ublarcvapp
// #include "ublarcvapp/UBImageMod/UBSplitDetector.h"
// #include "ublarcvapp/ubdllee/FixedCROIFromFlashAlgo.h"
// #include "ublarcvapp/ubdllee/FixedCROIFromFlashConfig.h"

// ROOT
//#include "TMessage.h"

// zmq-c++
//#include "zmq.hpp"

class DLLEEInterface;

class DLLEEInterface : public art::EDProducer, larcv::larcv_base {

public:

  explicit DLLEEInterface(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  DLLEEInterface(DLLEEInterface const &) = delete;
  DLLEEInterface(DLLEEInterface &&) = delete;
  DLLEEInterface & operator = (DLLEEInterface const &) = delete;
  DLLEEInterface & operator = (DLLEEInterface &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;
  void beginJob() override;
  void endJob() override;
  
private:

  // Declare member data here.
  int _verbosity;                    //< verbosity of module

  // larlite scanneralgo: to make larlite products used in DL algos
  larlite::ScannerAlgo _litemaker;   //< class used to make larlite products
  std::vector< std::string > _litemaker_opflash_producer_v;
  std::string _litemaker_ophitbeam_producer;
  void runLiteMaker( art::Event const& e, 
		     std::vector< larlite::event_opflash >& opflash_vv,
		     larlite::event_ophit& ophit_v );

  // supera/image formation: to make larcv products used in DL algos
  larcv::LArCVSuperaDriver _supera;  //< class used to make larcv products
  std::string _wire_producer_name;   //< name of wire product in art file
  std::string _supera_config;        //< supera config file
  int runSupera( art::Event& e, std::vector<larcv::Image2D>& wholeview_imgs );

  // ssnet image
  std::string _ssnet_producer;
  bool getSSNetFromArt( art::Event const& e, 
			const std::string ssnet_producer, 
			const std::vector<larcv::Image2D>& wholeview_v,
			std::vector< larcv::Image2D >& shower_v,
			std::vector< larcv::Image2D >& track_v );


  // image processing/splitting/cropping
  // Cropper_t   _cropping_method;
  // std::string _cropping_method_name;
  // ublarcvapp::UBSplitDetector _imagesplitter; //< full splitter
  // ublarcvapp::ubdllee::FixedCROIFromFlashAlgo _croifromflashalgo; //< croi from flash
  // bool        _save_detsplit_input;
  // bool        _save_detsplit_output;
  // std::string _opflash_producer_name;
  // std::vector<larcv::Image2D> _splitimg_v;              //< container holding split images
  // int runWholeViewSplitter( const std::vector<larcv::Image2D>& wholeview_v, 
  // 			    std::vector<larcv::Image2D>& splitimg_v,
  // 			    std::vector<larcv::ROI>&     splitroi_v );
  // int runCROIfromFlash( const std::vector<larcv::Image2D>& wholeview_v, 
  // 			art::Event& e,
  // 			std::vector<larcv::Image2D>& splitimg_v,
  // 			std::vector<larcv::ROI>&     splitroi_v );
  

  /// save to art::Event
  // void saveArtProducts( art::Event& ev,
  // 			const std::vector<larcv::Image2D>& wholeimg_v,
  // 			const std::vector<larcv::Image2D>& showermerged_v,
  // 			const std::vector<larcv::Image2D>& trackmerged_v );
  // std::vector<float> _pixelthresholds_forsavedscores;
  

  void saveLArCVProducts( std::vector<larcv::Image2D>& wholeview_imgs,
			  std::vector<larcv::Image2D>& shower_v,
			  std::vector<larcv::Image2D>& track_v );

  // =================
  //  Algorithms
  // =================

  // PMT Precuts
  larlite::LEEPreCut m_precutalgo;

  // Tagger/CROI
  larlitecv::TaggerCROIAlgoConfig m_taggeralgo_cfg;
  larlitecv::TaggerCROIAlgo*      m_taggeralgo;
  larlitecv::InputPayload         m_tagger_input;
  larlitecv::EmptyChannelAlgo     m_emptych_algo;

};

/**
 * constructor: configuration module
 *
 */
DLLEEInterface::DLLEEInterface(fhicl::ParameterSet const & p)
  : larcv::larcv_base("DLLEEInterface_module")
    // Initialize member data here.
{

  // verbosity
  // ----------
  _verbosity = p.get<int>("Verbosity",2);
  set_verbosity( (::larcv::msg::Level_t) _verbosity );
  _supera.set_verbosity( (::larcv::msg::Level_t) _verbosity );


  // -----------------
  // litemaker config
  // -----------------
  _litemaker_opflash_producer_v = p.get<std::vector<std::string> >("OpFlashBeamProducers");
  for ( auto const& opflash_producer : _litemaker_opflash_producer_v )
    _litemaker.Register( opflash_producer, larlite::data::kOpFlash );
  _litemaker_ophitbeam_producer = p.get<std::string>("OpHitBeamProducer");

  // ---------------
  // supera config
  // ---------------

  // product to get recob::wire data from
  _wire_producer_name = p.get<std::string>("WireProducerName");

  // name of supera configuration file
  _supera_config      = p.get<std::string>("SuperaConfigFile");

  std::string supera_cfg;
  cet::search_path finder("FHICL_FILE_PATH");
  if( !finder.find_file(_supera_config, supera_cfg) )
    throw cet::exception("DLLEEInterface") << "Unable to find supera cfg in "  << finder.to_string() << "\n";
  std::cout << "LOADING supera config: " << supera_cfg << std::endl;

  // configure supera (convert art::Event::CalMod -> Image2D)
  _supera.configure(supera_cfg);

  // ---------------
  //  ssnet
  // ---------------
  _ssnet_producer = p.get<std::string>("SSNetProducerName");

  // // check cfg content top level
  // larcv::PSet main_cfg = larcv::CreatePSetFromFile(supera_cfg).get<larcv::PSet>("ProcessDriver");

  // // get list of processors
  // std::vector<std::string> process_names = main_cfg.get< std::vector<std::string> >("ProcessName");
  // std::vector<std::string> process_types = main_cfg.get< std::vector<std::string> >("ProcessType");
  // larcv::PSet              process_list  = main_cfg.get<larcv::PSet>("ProcessList");
  // //larcv::PSet              split_cfg     = main_cfg.get<larcv::PSet>("UBSplitConfig"); 
  
  // if ( process_names.size()!=process_types.size() ) {
  //   throw std::runtime_error( "Number of Supera ProcessName(s) and ProcessTypes(s) is not the same." );
  // }


  // configure image splitter (fullimage into detsplit image)
  //_imagesplitter.configure( split_cfg );

  // ---------------
  //  LEEPreCuts
  // ---------------
  std::string precut_ftype = p.get<std::string>("PrecutConfig");
  if ( precut_ftype=="BNB" || precut_ftype=="MC" )
    m_precutalgo.setDefaults( larlite::LEEPreCut::kBNB );
  else if ( precut_ftype=="EXTBNB" )
    m_precutalgo.setDefaults( larlite::LEEPreCut::kEXTBNB );
  else if ( precut_ftype=="OVERLAY" )
    m_precutalgo.setDefaults( larlite::LEEPreCut::kOVERLAY );
  else
    throw cet::exception("DLLEEInterface") << "Unrecognized precut config. Choices [BNB,MC,EXTBNB,OVERLAY]" << std::endl;

  // ---------------
  //  Tagger
  // ---------------
  std::string      tagger_cfg;
  if( !finder.find_file( p.get<std::string>("TaggerConfigFile"), tagger_cfg) )
    throw cet::exception("DLLEEInterface") << "Unable to find tagger cfg in "  << finder.to_string() << "\n";
  LARCV_INFO() << "LOADING tagger config: " << tagger_cfg << std::endl;
  m_taggeralgo_cfg = larlitecv::TaggerCROIAlgoConfig::makeConfigFromFile( tagger_cfg );
  m_taggeralgo     = new larlitecv::TaggerCROIAlgo( m_taggeralgo_cfg );

}

/** 
 * produce DL network products
 *
 */
void DLLEEInterface::produce(art::Event & e)
{

  // get larlite products
  LARCV_INFO() << "Run the LiteMaker" << std::endl;
  std::vector< larlite::event_opflash > opflash_vv;
  larlite::event_ophit ophit_beam_v;
  runLiteMaker( e, opflash_vv, ophit_beam_v );

  // get larcv products
  LARCV_INFO() << "Run SUPERA" << std::endl;
  std::vector<larcv::Image2D> wholeview_v;
  int nwholeview_imgs = runSupera( e, wholeview_v );
  LARCV_INFO() << "number of wholeview images: " << nwholeview_imgs << std::endl;

  // recover ssnet info
  LARCV_INFO() << "Get SSNet From Art Event" << std::endl;
  std::vector< larcv::Image2D > shower_v;
  std::vector< larcv::Image2D > track_v;
  bool has_ssnet = getSSNetFromArt( e, _ssnet_producer, wholeview_v,
				    shower_v, track_v );
  LARCV_INFO() << "SSnet loaded: " << has_ssnet << std::endl;
  if ( has_ssnet )  {
    LARCV_INFO() << "  number of trackimgs=" << track_v.size() 
		 << " showerimgs=" << shower_v.size() << std::endl;
  }
  
  bool eventpass = true;

  // PMT Precuts
  eventpass = m_precutalgo.apply( ophit_beam_v );
  LARCV_INFO() << "PMT Precut Algo Result: " << eventpass << std::endl;

  // CROI/Tagger
  m_tagger_input.clear();
  
  // fill tagger input: adc image
  for ( auto const& img : wholeview_v )
    m_tagger_input.img_v.push_back( img );

  // fill tagger input: bad channels

  // fill tagger input: empty channels
  try {
    for ( auto const &img : wholeview_v ) {
      int p = img.meta().plane();
      larcv::Image2D emptyimg = m_emptych_algo.labelEmptyChannels( m_taggeralgo_cfg.emptych_thresh.at(p), img );
      m_tagger_input.gapch_v.emplace_back( std::move(emptyimg) );
    }
  }
  catch (std::exception& e) {
    std::cerr << "DLLEEInterface:: Error making empty channels: " << e.what() << std::endl;
    throw std::runtime_error("DLLEEInterface:  ERROR");
  }

  // Vertexer

  // Likelihood

  // // we often have to pre-process the image, e.g. split it.
  // // eventually have options here. But for now, wholeview splitter
  // std::vector<larcv::Image2D> splitimg_v;
  // std::vector<larcv::ROI>     splitroi_v;

  
  // int nsplit_imgs = 0;

  // switch (_cropping_method) {
  // case kWholeImageSplitter:
  //   nsplit_imgs = runWholeViewSplitter( wholeview_v, splitimg_v, splitroi_v );
  //   break;
  // case kFlashCROI:
  //   nsplit_imgs = runCROIfromFlash( wholeview_v, e, splitimg_v, splitroi_v );
  //   break;
  // }

  // LARCV_INFO() << "number of split images: " << nsplit_imgs << std::endl;

  // // containers for outputs
  // std::vector<larcv::Image2D> showerout_v;
  // std::vector<larcv::Image2D> trackout_v;


  // produce the art data product
  //saveArtProducts( e, wholeview_v, showermerged_v, trackmerged_v );

  // prepare the output
 
  
  // // save the wholeview images back to the supera IO
  // larcv::EventImage2D* ev_imgs  = (larcv::EventImage2D*) io.get_data( larcv::kProductImage2D, "wire" );
  // //std::cout << "wire eventimage2d=" << ev_imgs << std::endl;
  // ev_imgs->Emplace( std::move(wholeview_v) );

  // // save detsplit input
  // larcv::EventImage2D* ev_splitdet = nullptr;
  // if ( _save_detsplit_input ) {
  //   ev_splitdet = (larcv::EventImage2D*) io.get_data( larcv::kProductImage2D, "detsplit" );
  //   ev_splitdet->Emplace( std::move(splitimg_v) );
  // }

  // // save detsplit output
  // larcv::EventImage2D* ev_netout_split[2] = {nullptr,nullptr};
  // if ( _save_detsplit_output ) {
  //   ev_netout_split[0] = (larcv::EventImage2D*) io.get_data( larcv::kProductImage2D, "netoutsplit_shower" );
  //   ev_netout_split[1] = (larcv::EventImage2D*) io.get_data( larcv::kProductImage2D, "netoutsplit_track" );
  //   ev_netout_split[0]->Emplace( std::move(showerout_v) );
  //   ev_netout_split[1]->Emplace( std::move(trackout_v) );
  // }

  // // save merged out
  // larcv::EventImage2D* ev_merged[2] = {nullptr,nullptr};
  // ev_merged[0] = (larcv::EventImage2D*)io.get_data( larcv::kProductImage2D, "ssnetshower" );
  // ev_merged[1] = (larcv::EventImage2D*)io.get_data( larcv::kProductImage2D, "ssnettrack" );
  // ev_merged[0]->Emplace( std::move(showermerged_v) );
  // ev_merged[1]->Emplace( std::move(trackmerged_v) );

  saveLArCVProducts( wholeview_v, shower_v, track_v );
  
  LARCV_INFO() << "Event Passed: " << eventpass << std::endl;

}

/**
 * retrive SSNet data from the Art file
 *
 */
bool DLLEEInterface::getSSNetFromArt( art::Event const& e, const std::string ssnet_producer, 
				      const std::vector<larcv::Image2D>& wholeview_v,
				      std::vector< larcv::Image2D >& shower_v,
				      std::vector< larcv::Image2D >& track_v ) {
  
  LARCV_DEBUG() << "Get SSNet data" << std::endl;
  art::Handle< std::vector<ubdldata::pixeldata> > ssnet_handle;
  e.getByLabel( ssnet_producer, ssnet_handle );
  if ( !ssnet_handle.isValid() ) {
    std::cerr << "Attempted to load SSNet ubdldata::pixeldata. label=" << ssnet_producer << std::endl;
    throw cet::exception("DLLEEInterface") << "Could not load SSNet data, label=" << ssnet_producer << "." << std::endl;
  }
  LARCV_INFO() << "Successfully retreived art product" << std::endl;
  
  for ( auto const& pixdata : *ssnet_handle ) {
    if ( pixdata.dim_per_point()!=4 ) {
      throw cet::exception("DLLEEInterface") << "Not the expected number of points! expected=4 vs found=" << pixdata.dim_per_point() << std::endl;
    }
    LARCV_DEBUG() << "pixdata has " << pixdata.len() << " points with " << pixdata.dim_per_point() << " values per point" << std::endl;
    //std::vector<float> bbox = pixdata.as_vector_bbox(); // xmin, ymin, xmax, ymax
    //float width  = bbox[2]-bbox[0];
    //float height = bbox[3]-bbox[1]; 
    const larcv::ImageMeta& meta = wholeview_v.at( pixdata.id() ).meta();
    LARCV_DEBUG() << "storing pixdata into " << meta.dump();

    larcv::Image2D showerimg( meta );
    larcv::Image2D trackimg( meta );
    showerimg.paint(0);
    trackimg.paint(0);
    LARCV_DEBUG() << "made blank shower/track images" << std::endl;
    for ( size_t ipt=0; ipt<(size_t)pixdata.len(); ipt++ ) {
      std::vector<float> pix = pixdata.point(ipt);
      try {
	if ( pix[1]==meta.min_y() ) {
	  pix[1] += 0.5;
	}
	size_t col = meta.col( pix[0] );
	size_t row = meta.row( pix[1] );
	float shr = pix[2];
	float trk = pix[3];
	showerimg.set_pixel( row, col, shr );
	trackimg.set_pixel( row, col, trk );
      }
      catch (std::exception& e ) {
	LARCV_DEBUG() << "failure to load pixdata point[" << ipt << "] "  << std::endl;
	LARCV_DEBUG() << " error: " << e.what() << std::endl;
	LARCV_DEBUG() << "   nvalues=" << pix.size() << std::endl;
	LARCV_DEBUG() << "   (wire,tick)=(" << pix[0] << "," << pix[1] << ")" << std::endl;
      }
    }
    shower_v.emplace_back( std::move(showerimg) );
    track_v.emplace_back( std::move(trackimg) );
    LARCV_DEBUG() << "finished pixdata plane=" << pixdata.id() << std::endl;
  }

  return true;
}

/**
 * convert larsoft products to larlite
 *
 * we have to covert the following products
 *  -- opflash
 *
 * @param[in] e art::Event with wire data
 * 
 */
void DLLEEInterface::runLiteMaker( art::Event const& e, 
				   std::vector< larlite::event_opflash >& opflash_vv,
				   larlite::event_ophit& ophit_beam_v ) {

  // clear the event
  _litemaker.EventClear();

  // opflash products
  for ( auto const& opflash_producer : _litemaker_opflash_producer_v ) {
    art::Handle< std::vector<recob::OpFlash> > flash_handle;
    e.getByLabel( opflash_producer, flash_handle );
    if ( !flash_handle.isValid() ) {
      std::cerr << "Attempted to load OpFlash data. producer=" << opflash_producer << std::endl;
      throw cet::exception("DLLEEInterface") << "Could not load OpFlash data, label=" << opflash_producer << "." << std::endl;
    }
    else {
      LARCV_INFO() << "Loaded OpFlash data. label=" << opflash_producer << std::endl;
    }
    larlite::event_opflash ev_flash;
    _litemaker.ScanData( flash_handle, &ev_flash );
    LARCV_INFO() << "  event container " << opflash_producer << " has " << ev_flash.size() << std::endl;
    opflash_vv.emplace_back( std::move(ev_flash) );
  }
  
  // OpHit: beam
  art::Handle< std::vector<recob::OpHit> > ophit_beam_handle;
  e.getByLabel( _litemaker_ophitbeam_producer, ophit_beam_handle );
  if ( !ophit_beam_handle.isValid() ) {
    std::cerr << "Failed to load OpHit (beam) data. producer=" << _litemaker_ophitbeam_producer << std::endl;
    throw cet::exception("DLLEEInterface") << "Could not load OpHit(beam) data, producer=" << _litemaker_ophitbeam_producer << "." << std::endl;
  }
  try {
    _litemaker.ScanData( ophit_beam_handle, &ophit_beam_v );
    LARCV_INFO() << "  number of beam ophits: " << ophit_beam_v.size() << std::endl;
  }
  catch( std::exception& err ) {
    throw cet::exception("DLLEEInterface") << "Failure to convert ophit (beam) data into larlite object" << std::endl;
  }

  // Hit
}

/**
 * have supera make larcv images
 *
 * get recob::Wire from art::Event, pass it to supera
 * the _supera object will contain the larcv IOManager which will have images stored
 *
 * @param[in] e art::Event with wire data
 * @return int number of images to process by network
 *
 */
int DLLEEInterface::runSupera( art::Event& e, std::vector<larcv::Image2D>& wholeview_imgs ) {

  //
  // set data product
  // 
  // :wire  
  art::Handle<std::vector<recob::Wire> > data_h;
  //  handle sub-name
  if(_wire_producer_name.find(" ")<_wire_producer_name.size()) {
    e.getByLabel(_wire_producer_name.substr(0,_wire_producer_name.find(" ")),
		 _wire_producer_name.substr(_wire_producer_name.find(" ")+1,_wire_producer_name.size()-_wire_producer_name.find(" ")-1),
		 data_h);
  }else{ e.getByLabel(_wire_producer_name, data_h); }
  if(!data_h.isValid()) { 
    std::cerr<< "Attempted to load recob::Wire data: " << _wire_producer_name << std::endl;
    throw ::cet::exception("DLLEEInterface") << "Could not locate recob::Wire data!" << std::endl;
  }
  _supera.SetDataPointer(*data_h,_wire_producer_name);

  // execute supera
  bool autosave_entry = false;
  LARCV_INFO() << "process event: (" << e.id().run() << "," << e.id().subRun() << "," << e.id().event() << ")" << std::endl;
  _supera.process(e.id().run(),e.id().subRun(),e.id().event(), autosave_entry);

  // get the images
  auto ev_imgs  = (larcv::EventImage2D*) _supera.driver().io_mutable().get_data( larcv::kProductImage2D, "wire" );
  ev_imgs->Move( wholeview_imgs );
  
  return wholeview_imgs.size();
}

/**
 * take wholeview images (one per plane in vector) and turn into subimage crops (many per plane in vector)
 *
 * passes the images into the UBSplitDetector object (_imagesplitter)
 *
 * @param[in] wholeview_v vector of whole-view images. one per plane.
 * @param[inout] splitimg_v vector of cropped images. many per plane.
 * @param[inout] splitroi_v vector ROIs for split images. many  per plane.
 *
 **/
// int DLLEEInterface::runWholeViewSplitter( const std::vector<larcv::Image2D>& wholeview_v, 
// 				       std::vector<larcv::Image2D>& splitimg_v,
// 				       std::vector<larcv::ROI>&     splitroi_v ) {
  
//   // //
//   // // define the image meta and image
//   // //
//   // std::vector<larcv::ImageMeta> meta_v;
//   // for ( auto const& img : wholeview_v ) {
//   //   meta_v.push_back( img.meta() );
//   // }
  
//   //
//   // debug: dump the meta definitions for the wholeview images
//   //std::cout << "DLLEEInterface: superawire images made " << wholeview_v.size() << std::endl;
//   // for (auto& img : wholeview_v ) {
//   //   std::cout << img.meta().dump() << std::endl;
//   // }  
  
//   //
//   // split image into subregions
//   //
//   // splitimg_v.clear();
//   // splitroi_v.clear();
//   // try {
//   //   _imagesplitter.process( wholeview_v, splitimg_v, splitroi_v );
//   // }
//   // catch (std::exception& e ) {
//   //   throw cet::exception("DLLEEInterface") << "error splitting image: " << e.what() << std::endl;
//   // }

//   // return splitimg_v.size();

// }

/**
 * produce subimage crops based on the location of the in-time flash
 *
 * passes the images into an instance of FixedCROIFromFlashAlgo (_croifromflashalgo member)
 *
 * @param[in] wholeview_v vector of whole-view images. one per plane.
 * @param[in] e art::Event from which we will get the opflash objects
 * @param[inout] splitimg_v vector of cropped images. many per plane.
 * @param[inout] splitroi_v vector ROIs for split images. many  per plane.
 *
 **/
// int DLLEEInterface::runCROIfromFlash( const std::vector<larcv::Image2D>& wholeview_v, 
// 				   art::Event& e,
// 				   std::vector<larcv::Image2D>& splitimg_v,
// 				   std::vector<larcv::ROI>&     splitroi_v ) {

//   // first get the opflash objects from art
//   art::Handle<std::vector<recob::OpFlash> > data_h;
//   //  handle sub-name
//   if(_opflash_producer_name.find(" ")<_opflash_producer_name.size()) {
//     e.getByLabel(_opflash_producer_name.substr(0,_opflash_producer_name.find(" ")),
// 		 _opflash_producer_name.substr(_opflash_producer_name.find(" ")+1,_opflash_producer_name.size()-_opflash_producer_name.find(" ")-1),
// 		 data_h);
//   }else{ e.getByLabel(_opflash_producer_name, data_h); }

//   if(!data_h.isValid()) { 
//     std::cerr<< "Attempted to load recob::Opflash data: " << _opflash_producer_name << std::endl;
//     throw cet::exception("DLLEEInterface") << "Could not locate recob::Opflash data!" << std::endl;
//   }
//   //std::cout << "Loaded opflash data" << std::endl;

//   const float usec_min = 190*0.015625;
//   const float usec_max = 320*0.015625;

//   std::vector<larlite::opflash> intime_flashes_v;
//   for ( auto const& artflash : *data_h ) {

//     if ( artflash.Time()<usec_min || artflash.Time()>usec_max ) continue;

//     // hmm, there is no getter for the number of entries in the PE vector. 
//     // that seems wrong. we cheat about the number of PEs for now
//     // next we have to make larlite objects
//     std::vector<double> pe_v(32,0.0);
//     for ( size_t ich=0; ich<32; ich++ ) {
//       pe_v[ich] = artflash.PE(ich);
//     }
//     larlite::opflash llflash( artflash.Time(),    artflash.TimeWidth(),
// 			      artflash.AbsTime(), artflash.Frame(),
// 			      pe_v,
// 			      1, 1, 1,
// 			      artflash.YCenter(), artflash.YWidth(),
// 			      artflash.ZCenter(), artflash.ZWidth(),
// 			      artflash.WireCenters(),
// 			      artflash.WireWidths() );
//     intime_flashes_v.emplace_back( std::move(llflash) );
//   }
//   //std::cout << "converted into larlite opflash" << std::endl;

//   // pass them to the algo to get CROI    
//   for ( auto& llflash : intime_flashes_v ) {
//     std::vector<larcv::ROI> flashrois = _croifromflashalgo.findCROIfromFlash( llflash );
//     for ( auto& roi : flashrois )
//       splitroi_v.emplace_back( std::move(roi) );
//   }
//   //std::cout << "create roi from flash" << std::endl;

//   // cropout the regions
//   size_t nplanes = wholeview_v.size();
//   size_t ncrops = 0;
//   for (size_t plane=0; plane<nplanes; plane++ ) {
//     const larcv::Image2D& wholeimg = wholeview_v[plane];
//     for ( auto& roi : splitroi_v ) {
//       const larcv::ImageMeta& bbox = roi.BB(plane);
//       //std::cout << "crop from the whole image" << std::endl;
//       try {
// 	larcv::Image2D crop = wholeimg.crop( bbox );
// 	splitimg_v.emplace_back( std::move(crop) );
//       }
//       catch( std::exception& e ) {
// 	throw cet::exception("DLLEEInterface") << "error in cropping: " << e.what() << std::endl;
//       }
//       ncrops++;
//     }
//   }
//   //std::cout << "cropped the regions: total " << ncrops << std::endl;

//   // done
//   return (int)ncrops;
// }


// /**
//  * create ubdldata::pixeldata objects for art::Event
//  *
//  *
//  */
// void DLLEEInterface::saveArtProducts( art::Event& ev, 
// 				   const std::vector<larcv::Image2D>& wholeimg_v,
// 				   const std::vector<larcv::Image2D>& showermerged_v,
// 				   const std::vector<larcv::Image2D>& trackmerged_v ) {

//   std::unique_ptr< std::vector<ubdldata::pixeldata> > ppixdata_v(new std::vector<ubdldata::pixeldata>);

//   for ( auto const& adc : wholeimg_v ) {
//     int planeid           = (int)adc.meta().plane();
//     float pix_threshold   = _pixelthresholds_forsavedscores.at(planeid);
//     auto const& showerimg = showermerged_v.at(planeid);
//     auto const& trackimg  = trackmerged_v.at(planeid);

//     std::vector< std::vector<float> > pixdata_v;
//     // we reserve enough space to fill the whole image, but we shouldn't use all of the space
//     pixdata_v.reserve( adc.meta().rows()*adc.meta().cols() );

//     size_t npixels = 0;
//     for ( size_t r=0; r<adc.meta().rows(); r++ ) {
//       float tick = adc.meta().pos_y(r);
//       for ( size_t c=0; c<adc.meta().cols(); c++ ) {

// 	// each pixel
	
// 	if ( adc.pixel(r,c)<pix_threshold ) continue;

// 	float wire=adc.meta().pos_x(c);

// 	std::vector<float> pixdata = { (float)wire, (float)tick, 
// 				       (float)showerimg.pixel(r,c), (float)trackimg.pixel(r,c) };
	
// 	pixdata_v.push_back( pixdata );

// 	npixels++;
//       }
//     }
  
//     ubdldata::pixeldata out( pixdata_v,
// 			     adc.meta().min_x(), adc.meta().min_y(), 
// 			     adc.meta().max_x(), adc.meta().max_y(),
// 			     (int)adc.meta().cols(), (int)adc.meta().rows(),
// 			     (int)adc.meta().plane(), 4, 0 );

//     ppixdata_v->emplace_back( std::move(out) );
//   }

  
//   ev.put( std::move(ppixdata_v) );
// }

/**
 * save LArCV products to file
 *
 */
void DLLEEInterface::saveLArCVProducts( std::vector<larcv::Image2D>& wholeview_v,
					std::vector<larcv::Image2D>& shower_v,
					std::vector<larcv::Image2D>& track_v ) {
  
  larcv::IOManager& io_larcv = _supera.driver().io_mutable();

  // save wholeview ADC images
  LARCV_DEBUG() << "Save ADC" << std::endl;
  auto ev_imgs  = (larcv::EventImage2D*) io_larcv.get_data( larcv::kProductImage2D, "wire" );
  ev_imgs->Emplace( std::move(wholeview_v) );

  // save shower score images
  LARCV_DEBUG() << "Save shower images" << std::endl;
  auto ev_shr  = (larcv::EventImage2D*) io_larcv.get_data( larcv::kProductImage2D, "ssnetshower" );
  ev_shr->Emplace( std::move(shower_v) );
  
  // save track score images
  LARCV_DEBUG() << "Save track images" << std::endl;
  auto ev_trk  = (larcv::EventImage2D*) io_larcv.get_data( larcv::kProductImage2D, "ssnettrack" );
  ev_trk->Emplace( std::move(track_v) );

  // save entry
  LARCV_INFO() << "saving LARCV entry" << std::endl;
  io_larcv.save_entry();
  
  // we clear entries ourselves
  LARCV_INFO() << "clearing entry" << std::endl;
  io_larcv.clear_entry();

}


/**
 * 
 * overridden method: setup class before job is run
 *
 * 
 */
void DLLEEInterface::beginJob()
{

  LARCV_DEBUG() << "initialize Supera" << std::endl;
  _supera.initialize();

}

/**
 * 
 * overridden method: setup class after all events are run
 *
 * 
 */
void DLLEEInterface::endJob()
{
  LARCV_DEBUG() << "finalize IOmanager" << std::endl;
  _supera.finalize();
  LARCV_DEBUG() << "destroy tagger algo" << std::endl;
  delete m_taggeralgo;
}

DEFINE_ART_MODULE(DLLEEInterface)
