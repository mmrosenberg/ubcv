///////////////////////////////////////////////////////////////////////
/// \file  LArRawInputDriverUBooNE.cxx
/// \brief Source to convert raw binary files to root files
/// \Original Authors
/// \version $Id: LArRawInputDriver.h,v 1.7 2010/01/14 19:20:33 brebel Exp $
/// \author  brebel@fnal.gov, soderber@fnal.gov
/// \MicroBooNE Author: jasaadi@fnal.gov, zarko@fnal.gov (with much help from Wes and Eric)
////////////////////////////////////////////////////////////////////////

//LArSoft 
#include "uboone/RawData/utils/LArRawInputDriverUBooNE.h"
#include "RawData/RawDigit.h"
#include "RawData/TriggerData.h"
#include "RawData/DAQHeader.h"
#include "RawData/BeamInfo.h"
#include "RawData/OpDetWaveform.h"
#include "Geometry/Geometry.h"
#include "SummaryData/RunData.h"
#include "Utilities/TimeService.h"
#include "Utilities/ElecClock.h" // lardata
#include "OpticalDetectorData/OpticalTypes.h" // lardata -- I want to move the enums we use back to UBooNE as they are UBooNE-specific

//ART, ...
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "art/Utilities/Exception.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//uboone datatypes

// uboonecode
#include "uboone/Geometry/UBOpChannelTypes.h"
#include "uboone/Geometry/UBOpReadoutMap.h"

#include "datatypes/raw_data_access.h"

//boost
//#include <boost/archive/binary_iarchive.hpp>
#include <boost/algorithm/string.hpp>

//root
#include "TH1D.h"

//other
#include <libpq-fe.h>

extern "C" {
#include <sys/types.h>
}

#include <bitset>
#include <iostream>

namespace ubdaq=gov::fnal::uboone::datatypes;

// +++ Blatant stealing from LongBo
namespace lris {

  // ======================================================================
  LArRawInputDriverUBooNE::LArRawInputDriverUBooNE(fhicl::ParameterSet const & ps, 
                                                   art::ProductRegistryHelper &helper,
                                                   art::SourceHelper const &pm)
    :
    fSourceHelper(pm),
    fCurrentSubRunID(),
    fEventCounter(0),
    fHuffmanDecode(ps.get<bool>("huffmanDecode",false)),
    fChannelMap( art::ServiceHandle<util::DatabaseUtil>()->GetUBChannelMap() )
  {
    mf::LogInfo("")<<"Fetched channel map from DB";

    helper.reconstitutes<raw::DAQHeader,              art::InEvent>("daq");
    helper.reconstitutes<std::vector<raw::RawDigit>,  art::InEvent>("daq");
    helper.reconstitutes<raw::BeamInfo,               art::InEvent>("daq");
    helper.reconstitutes<std::vector<raw::Trigger>,   art::InEvent>("daq");
    registerOpticalData( helper ); //helper.reconstitutes<std::vector<raw::OpDetWaveform>,art::InEvent>("daq");

    //if ( fHuffmanDecode )
    tpc_crate_data_t::doDissect(true); // setup for decoding

    art::ServiceHandle<art::TFileService> tfs;
    //initialize beam histograms specified in fhicl file
    art::TFileDirectory tfbeamdir = tfs->mkdir( "Beam" );
    std::vector<std::string> beam_hist=ps.get<std::vector<std::string> >("beam_histograms");
    for ( auto it : beam_hist ) {
      std::vector<std::string> hist;
      boost::split(hist, it, boost::is_any_of(","));
      if (hist.size() != 4)
        mf::LogWarning("") << "Bad definition in fhicl file for histogram "<<hist.at(0)<<". Ignoring it.";
      else {
        TH1D* h=tfbeamdir.make<TH1D>(hist[0].c_str(),hist[0].c_str(),
                                      atoi(hist[1].c_str()),atof(hist[2].c_str()),atof(hist[3].c_str()));
        std::pair<std::string, TH1D*> p(hist[0],h);
        fHistMapBeam.insert(p);
      }
    }
  }
  
  
  // ======================================================================
  void LArRawInputDriverUBooNE::closeCurrentFile()  
  {    
    mf::LogInfo(__FUNCTION__)<<"File boundary (processed "<<fEventCounter<<" events)"<<std::endl;
    fCurrentSubRunID.flushSubRun();
    fEventCounter=0;
    fInputStream.close();
  }
    
  // ======================================================================
  void LArRawInputDriverUBooNE::readFile(std::string const &name,
                                         art::FileBlock* &fb)
  {
    // Fill and return a new Fileblock.
    fb = new art::FileBlock(art::FileFormatVersion(1, "LArRawInput 2011a"),
                            name);
    
    fInputStream.open(name.c_str(),std::ios_base::in | std::ios_base::binary);
    
    
    // Throwing an exception if the file fails to open
    if( !fInputStream.is_open() ) {
      throw art::Exception( art::errors::FileReadError )
        << "failed to open input file " << name << std::endl;
    }

    return;

    //seek to the end of file, check the end word, check number of events and sizes
    //read in the end of file word first
    uint16_t end_of_file_marker;
    fInputStream.seekg( -1*sizeof(uint16_t) , std::ios::end);
    fInputStream.read( (char*)&end_of_file_marker , sizeof(uint16_t));

    if(end_of_file_marker != 0xe0f0){
      //throw art::Exception( art::errors::FileReadError )
      std::cout << "File "<<name<<" has incorrect end of file marker. "<< end_of_file_marker<<std::endl;
    }
    fInputStream.seekg(std::ios::beg);
    
    return;
    /*
    //get number of events from word at end of file
    fInputStream.seekg( -1*(sizeof(uint16_t)+sizeof(uint32_t)), std::ios::end);
    fInputStream.read( (char*)&fNumberOfEvents , sizeof(uint32_t));

    fNumberOfEvents = 10;
    //now get all of the event sizes, 
    uint32_t tmp_event_size;
    std::streampos count_event_size=0;
    fInputStream.seekg( -1*(sizeof(uint16_t)+(fNumberOfEvents+1)*sizeof(uint32_t)), std::ios::end);

    for(uint32_t i=0; i<fNumberOfEvents; i++){
      // since we want the beginning, push back the event size before incrementing it
      fEventLocation.push_back(count_event_size);
      fInputStream.read( (char*)&tmp_event_size , sizeof(uint32_t));
      count_event_size += tmp_event_size;
    }
    fEventLocation.push_back(count_event_size);
    fInputStream.seekg(std::ios::beg);

    mf::LogInfo("")<<"Opened file "<<name<<" with "<<fNumberOfEvents<<" event(s)";
    */
  }


  // =====================================================================
  void LArRawInputDriverUBooNE::registerOpticalData( art::ProductRegistryHelper &helper ) {
    // we make a data product for each category of channels
    fPMTdataProductNames.clear();
    for ( unsigned int cat=0; cat<(unsigned int)opdet::NumUBOpticalChannelCategories; cat++ ) {
      std::stringstream ss;
      ss << "pmtreadout" << opdet::UBOpChannelEnumName( (opdet::UBOpticalChannelCategory_t)cat );
      helper.reconstitutes<std::vector<raw::OpDetWaveform>,art::InEvent>(ss.str()); 
      fPMTdataProductNames.insert( std::make_pair( (opdet::UBOpticalChannelCategory_t)cat, ss.str() ) );
    }
  }

  // =====================================================================
  void LArRawInputDriverUBooNE::putPMTDigitsIntoEvent( std::map< opdet::UBOpticalChannelCategory_t, std::unique_ptr< std::vector<raw::OpDetWaveform> > >& pmtdigitlist, 
						       art::EventPrincipal* &outE ) {
    for ( unsigned int cat=0; cat<(unsigned int)opdet::NumUBOpticalChannelCategories; cat++ ) {
      
      art::put_product_in_principal(std::move( pmtdigitlist[(opdet::UBOpticalChannelCategory_t)cat]  ),
				    *outE,
				    fPMTdataProductNames[ (opdet::UBOpticalChannelCategory_t)cat ]); // Module label
    }
    
  }
  
  // =====================================================================
  bool LArRawInputDriverUBooNE::readNext(art::RunPrincipal* const &/*inR*/,
                                         art::SubRunPrincipal* const &/*inSR*/,
                                         art::RunPrincipal* &outR,
                                         art::SubRunPrincipal* &outSR,
                                         art::EventPrincipal* &outE)
  {
    mf::LogInfo(__FUNCTION__)<<"Attempting to read event: "<<fEventCounter<<std::endl;
    // Create empty result, then fill it from current file:
    std::unique_ptr<raw::DAQHeader> daq_header(new raw::DAQHeader);
    std::unique_ptr<std::vector<raw::RawDigit> >  tpc_raw_digits( new std::vector<raw::RawDigit>  );
    std::unique_ptr<raw::BeamInfo> beam_info(new raw::BeamInfo);
    std::unique_ptr<std::vector<raw::Trigger>> trig_info( new std::vector<raw::Trigger> );
    std::map< opdet::UBOpticalChannelCategory_t, std::unique_ptr< std::vector<raw::OpDetWaveform> > > pmt_raw_digits;
    for ( unsigned int opdetcat=0; opdetcat<(unsigned int)opdet::NumUBOpticalChannelCategories; opdetcat++ ) {
      pmt_raw_digits.insert( std::make_pair( (opdet::UBOpticalChannelCategory_t)opdetcat, std::unique_ptr< std::vector<raw::OpDetWaveform> >(  new std::vector<raw::OpDetWaveform> ) ) );
    }

    bool res=false;

    res=processNextEvent(*tpc_raw_digits, pmt_raw_digits, *daq_header, *beam_info, *trig_info );

    if (res) {
      fEventCounter++;
      art::RunNumber_t rn = daq_header->GetRun();//+1;
      art::Timestamp tstamp = daq_header->GetTimeStamp();
      art::SubRunID newID(rn, daq_header->GetSubRun());
      if (fCurrentSubRunID.runID() != newID.runID()) { // New Run
        outR = fSourceHelper.makeRunPrincipal(rn, tstamp);
      }
      if (fCurrentSubRunID != newID) { // New SubRun
        outSR = fSourceHelper.makeSubRunPrincipal(rn,
                                                  daq_header->GetSubRun(),
                                                  tstamp);
        fCurrentSubRunID = newID;        
      }
      /*
      std::cout<<"\033[93mAbout to make a principal for run: " << fCurrentSubRunID.run()
	       <<" subrun: " << fCurrentSubRunID.subRun()
	       <<" event: " << daq_header->GetEvent()
	       <<"\033[00m"<< std::endl;
      */
      outE = fSourceHelper.makeEventPrincipal(fCurrentSubRunID.run(),
					      fCurrentSubRunID.subRun(),
					      daq_header->GetEvent(),
					      tstamp);
      //std::cout<<"\033[93mDone\033[00m"<<std::endl;

      // Put products in the event.
      art::put_product_in_principal(std::move(tpc_raw_digits),
                                    *outE,
                                    "daq"); // Module label
      art::put_product_in_principal(std::move(daq_header),
                                    *outE,
                                    "daq"); // Module label
      art::put_product_in_principal(std::move(beam_info),
                                    *outE,
                                    "daq"); // Module label
      art::put_product_in_principal(std::move(trig_info),
				    *outE,
				    "daq"); // Module label
      putPMTDigitsIntoEvent( pmt_raw_digits, outE );
     
    }

    return res;

  }

  // =====================================================================
  bool LArRawInputDriverUBooNE::processNextEvent(std::vector<raw::RawDigit>& tpcDigitList,
                                                 std::map< opdet::UBOpticalChannelCategory_t, std::unique_ptr<std::vector<raw::OpDetWaveform>> >& pmtDigitList,
                                                 raw::DAQHeader& daqHeader,
                                                 raw::BeamInfo& beamInfo,
						 std::vector<raw::Trigger>& trigInfo)
  {       
    //try {
      boost::archive::binary_iarchive ia(fInputStream); 
      ubdaq::ub_EventRecord event_record;  
      ia >> event_record;
      //std::cout<<event_record.debugInfo()<<std::endl;
      //set granularity 
      //      event_record.updateIOMode(ubdaq::IO_GRANULARITY_CHANNEL);
      
      fillDAQHeaderData(event_record, daqHeader);
      fillTPCData(event_record, tpcDigitList);
      fillPMTData(event_record, pmtDigitList);
      fillBeamData(event_record, beamInfo);
      fillTriggerData(event_record, trigInfo);
      //std::cout<<"Done ProcessNextEvent..."<<std::endl;
      /*
    } catch (...) {
      //throw art::Exception( art::errors::FileReadError )
      std::cout<< "\033[93mFailed to read the event.\033[00m\n"<< std::endl;
      return false;
    }
      */  
    return true;
  }
  
  // =====================================================================
  void LArRawInputDriverUBooNE::fillDAQHeaderData(ubdaq::ub_EventRecord& event_record,
                                                  raw::DAQHeader& daqHeader)
  {
      ubdaq::ub_GlobalHeader global_header = event_record.getGlobalHeader();
      
      // art::Timestamp is an unsigned long long. The conventional 
      // use is for the upper 32 bits to have the seconds since 1970 epoch 
      // and the lower 32 bits to be the number of nanoseconds within the 
      // current second.
      // (time_t is a 64 bit word)

      uint32_t seconds=global_header.getSeconds();
      uint32_t nano_seconds=global_header.getNanoSeconds();
      time_t mytime = ((time_t)seconds<<32) | nano_seconds;

      //\/      uint32_t subrun_num = global_header->getSubrunNumber();
      
      daqHeader.SetStatus(1);
      daqHeader.SetFileFormat(global_header.getRecordType());
      daqHeader.SetSoftwareVersion(global_header.DAQ_version_number);
      daqHeader.SetRun(global_header.getRunNumber());
      daqHeader.SetSubRun(global_header.getSubrunNumber());
      
      //\/ Add the subRun number too!
      daqHeader.SetEvent(global_header.getEventNumber()+1);
      daqHeader.SetTimeStamp(mytime);

      /// \todo: What is the "fixed word" ? Leaving it unset for now
      /// \todo: What is the "spare word" ? Leaving it unset for now
      //daqHeader.SetFixedWord(h1.header);
      //daqHeader.SetSpareWord(h1.spare);
  }

  // =====================================================================
  void LArRawInputDriverUBooNE::fillTPCData(ubdaq::ub_EventRecord& event_record,
                                            std::vector<raw::RawDigit>& tpcDigitList)
  {    
      // ### Swizzling to get the number of channels...trying the method used in write_read.cpp
      // ### provided by Wes --- About the data:
      // ### The format of the data is in levels: crate, card, channel.
      // ### Level 1: The event record contains a map of (crateHeader,crateData) pairs.
      // ### Level 2: Each crateData object may contain a map of (cardHeader,cardData) pairs.
      // ### Level 3: Each cardData object may contain a map of (int,channelData) pairs. The int
      // ### is the channel number.
      
      //get the seb map, and do a loop over all sebs/crates

    for( auto const& seb_it : event_record.getTPCSEBMap()) {    // I think auto should be tpc_map_t::const_iterator  -NJT

          
        //get the crateHeader/crateData objects
      //        ubdaq::crateHeader crate_header = seb_it->first;
      //        ubdaq::crateData crate_data = seb_it->second;
      //      int tpc_seb_num = seb_it.first;
      tpc_crate_data_t const& tpc_crate = seb_it.second; // (ub_TPC_CrateData_v6)
      int crate_number = seb_it.first; // confirmed this is correct. for now, crate's are not given their ID number to store and then retrieve.
      
      if ( !tpc_crate.wasDissected() ) {
	std::cerr << "Warning crate data corrupted! Skipping." << std::endl;
	tpc_crate.dissectionException().what();
	continue;
      }

        // Get Time information:
        //uint32_t sebTSec = crate_header.getSebTimeSec();
        //std::cout << "Seb Time (sec) : " << sebTSec << std::endl;
        //crate_header_t crHeader = crate_header.getCrateHeader();
        // GPStime in UNIX second/micro/nano info
        //gps_time_t GPStime = crHeader.gps_time;
        // DAQtime is time of last update of GPS time (in frame, sample, div)
        //tbclkub_t  DAQtime = crHeader.daqClock_time;
        //std::cout << "GPS Time seconds: " << GPStime.second << std::endl;
        //std::cout << "DAQ Frame: " << DAQtime.frame << "\tSample: " << DAQtime.sample << std::endl;

      //      auto const& tpc_crate_header = tpc_crate.header();    
      //      auto const& tpc_crate_trailer = tpc_crate.trailer();  

      //Special to the crate, there is a special header that the DAQ attaches. You can access this
      //like so. The type here is a unique ptr to a ub_CrateHeader_v6 struct. That has useful info
      //like the local host time, which may or may not be set properly right now...
      auto const& tpc_crate_DAQ_header = tpc_crate.crateHeader(); // I think auto should be tpc_crate_data_t::ub_CrateHeader_t --NJT
      //     ub_LocalHostTime this_time = tpc_crate_DAQ_header->local_host_time;
      
      //The Crate Data is split up into Cards. You use the "getCards()" command to get access to
      //each of those. Note that calling this function will dissect the data if it has not already
      //been dissected (debugInfo() calls getCards()). You can do a look over the cards like so:
      for(auto const& card : tpc_crate.getCards()){  // This auto is tpc_crate_data_t::card_t

	if ( !card.wasDissected() ) {
	  std::cerr << "Warning card data corrupted! Skipping." << std::endl;
	  card.dissectionException().what();
	  continue;
	}

        //The format here is similar to the crate! There's a header (which is a ub_TPC_CardHeader_v*
        //object), and technically a trailer (though here it's empty!).
	//	auto const& tpc_card_header = card.header();   
	//	auto const& tpc_card_trailer = card.trailer(); 

        //Of course, you can probe for information in the card header. You'll have to find the appropriate
        //header file to know what type you have, but again, these will follow typical practice. And, you
        //can always use debugInfo to not only print the info, but it tells you the type.
        // auto const this_event_number = card.getEvent(); /// auto are ints here
        // auto const this_frame_number = card.getFrame(); /// auto are ints here


        //And, you guessed it, the tpc card data is split up into one more level: by channel.
        for(auto const& channel : card.getChannels()){ // auto here tpc_crate_data_t::card_t::card_channel_type


	  if ( !channel.wasDissected() ) {
	    std::cerr << "Warning channel data corrupted! Skipping." << std::endl;
	    //channel.dissectionException().what();
	    continue;
	  }

	  //There's a header and trailer here. Remember these are just uint16_t, that contain the
	  //channel number.
	  // auto const& tpc_channel_header = channel.header();   // unused
	  // auto const& tpc_channel_trailer = channel.trailer(); // unsued
	  
	  //The channel object (ub_MarkedRawChannelData) has a method for returning the channel.
	  //You can look at the other objects too (like ub_MarkedRawCardData) and see methods of
	  //use there as well.
	  auto const tpc_channel_number = channel.getChannelNumber(); // auto is int here
                        

            // output:
            std::vector<short> adclist;
	    size_t chdsize(0); 

	    //Huffman decoding
	    if (fHuffmanDecode) {
              channel.decompress(adclist); // All-in-one call.
	      uint16_t frailer = channel.getChannelTrailerWord();
	      if ( adclist.size()<9595 ) {
		short lachadawin = adclist.at( adclist.size()-1 );
		std::vector<short> kaxufix = decodeChannelTrailer( (unsigned short)lachadawin, (unsigned short)frailer );
		for ( auto& it : kaxufix )
		  adclist.emplace_back( it );
		//std::cout << "trailer: " << trailer_word << std::endl;
		//short thecheat = adclist.at( adclist.size()-1 );
		//while ( adclist.size()<9595 ) {
		//adclist.push_back( thecheat );
		//}
	      }
	      chdsize = adclist.size();
            } else {
              const ub_RawData& chD = channel.data(); 
	      // chdsize=(chD.getChannelDataSize()/sizeof(uint16_t));    
	      // chdsize = chD.size()/sizeof(uint16_t);    
	      chdsize = chD.size();
              adclist.reserve(chD.size()); // optimize
              for(ub_RawData::const_iterator it = chD.begin(); it!= chD.end(); it++) {
                adclist.push_back(*it);
              }
	      //              chD.decompress();
            }
	    
	    //int crate_number = tpc_crate.crateHeader()->crate_number;
	    util::UBDaqID daqId( crate_number, card.getModule(), tpc_channel_number);

            int ch=0;
	    auto it_chsearch = fChannelMap.find(daqId);
            if ( it_chsearch!=fChannelMap.end() ){
              ch=(*it_chsearch).second;
	      //              fChannelMap[daqId];
              //              wire=fWireMap[daqId];
              //              pl=fPlaneMap[daqId];
            }
	    else {
	      if ( ( crate_number==1 && card.getModule()==8 && (tpc_channel_number>=32 && tpc_channel_number<64) ) ||
		   ( crate_number==9 && card.getModule()==5 && (tpc_channel_number>=32 && tpc_channel_number<64) ) ) {
		// As of 6/22/2016: We expect these FEM channels to have no database entry.
		continue; // do not write to data product
	      }
	      else {
		// unexpected channels are missing. throw.
		char warn[256];
		sprintf( warn, "Warning DAQ ID not found ( %d, %d, %d )!", crate_number, card.getModule(), tpc_channel_number );
		throw std::runtime_error( warn );
	      }
	    }
            //\todo fix this once there is a proper channel table
            // else{
            //   //continue;
            //   ch=10000*tpc_crate.crateHeader()->crate_number
            //     +100*card.getModule()
            //     +tpc_channel_number;
            // }

            //if (int(ch) >= 8254)
            // continue;
            //raw::Compress_t compression=raw::kHuffman;
            //if (fHuffmanDecode) compression=raw::kNone;
	    raw::Compress_t compression=raw::kNone; // as of June 19,2015 compression not used by the DAQ. Data stored is uncompressed.
	    if ( adclist.size()!=9595 ) {
	      char warn[256];
	      sprintf( warn, "Error: Number of ADCs in (crate,slot,channel)=( %d, %d, %d ) does not equal 9595!", crate_number, card.getModule(), tpc_channel_number );
	      throw std::runtime_error( warn );
	    }
	    
            raw::RawDigit rd(ch,chdsize,adclist,compression);
            tpcDigitList.push_back(rd);
	    
            /*
            std::cout << ch << "\t"
                      << int(crate_header.getCrateNumber()) << "\t" 
                      << card_header.getModule() << "\t"
		      << channel_number << "\t"
                      << rms << std::endl;
            */

          }//<--End channel_it for loop
        }//<---End card_it for loop
      }//<---End seb_it for loop

    if ( tpcDigitList.size()!=8256 ) {
      char warn[256];
      sprintf( warn, "Error: Number of channels saved (%d) did not match the expectation (8256)!", (int)tpcDigitList.size() );
      //throw std::runtime_error( warn );
    }
  }

  // =====================================================================
  void LArRawInputDriverUBooNE::fillPMTData(ubdaq::ub_EventRecord& event_record,
					    std::map< opdet::UBOpticalChannelCategory_t, std::unique_ptr<std::vector<raw::OpDetWaveform>> >& pmtDigitList )
  {
    //fill PMT data

      // MODIFIED by Nathaniel Sat May 16, to use my new version of datatypes (v6_08, on branch master)
    
    //crate -> card -> channel -> window

    ::art::ServiceHandle<geo::Geometry> geom;
    ::art::ServiceHandle< util::TimeService > timeService;
    ::art::ServiceHandle<geo::UBOpReadoutMap> ub_pmt_channel_map;
    
    using namespace gov::fnal::uboone::datatypes;
    
    auto const seb_pmt_map = event_record.getPMTSEBMap();
    
    for(auto const& it:  seb_pmt_map) {
      pmt_crate_data_t const& crate_data = it.second;
      //      int crate_number = crate_data.crateHeader()->crate_number;
      
      if ( !crate_data.wasDissected() ) {
	std::cerr << "Warning PMT crate data corrupted! Skipping." << std::endl;
	continue;
      }

      //now get the card map (for the current crate), and do a loop over all cards
      std::vector<pmt_crate_data_t::card_t> const& cards = crate_data.getCards();
       
      for( pmt_crate_data_t::card_t const& card_data : cards ) {
        
	if ( !card_data.wasDissected() ) {
	  std::cerr << "Warning PMT card data corrupted! Skipping." << std::endl;
	  continue;
	}

	//        int card_number = card_data.getModule();
        
        // nathaniel's version of datatypes:
        for(auto const& channel_data : card_data.getChannels() ) { // auto here is pmt_crate_data_t::card_t::card_channel-type

	  // if ( !channel_data.wasDissected() ){
	  //   std::cerr << "Warning PMT channel data corrupted! Skipping." << std::endl;
	  //   continue;
	  // }

          int channel_number = channel_data.getChannelNumber();
          
          //now get the windows
          auto const& windows = channel_data.getWindows();  // auto here is std::vector<ub_PMT_WindowData_v6>
          for(const auto& window: windows ) {               // auto here is ub_PMT_WindowData_v6
            const auto& window_header = window.header();    // auto here is ub_PMT_WindowHeader_v6
            const ub_RawData& window_data = window.data();
            size_t win_data_size=window_data.size();
            
            // //\todo check category, time & frame
            // optdata::Optical_Category_t category = optdata::kUndefined;
            // if ((window_header.getDiscriminantor()&0x04)==0x04) {
            //   category=optdata::kBeamPMTTrigger;
            // } else {
            //   category=optdata::kCosmicPMTTrigger;
            // }
	    // tmw: In this new scheme, category is no longer needed (5/26/15)
            
            optdata::TimeSlice_t time=window_header.getSample();
            optdata::Frame_t frame=window_header.getFrame();
	    unsigned int data_product_ch_num = ub_pmt_channel_map->GetChannelNumberFromCrateSlotFEMCh( crate_data.crateHeader()->crate_number, card_data.getModule(), channel_number );
	    //int crate_number = crate_data.crateHeader()->crate_number; 
	    //std::cout << "fill (CSF): " << crate_number << ", " << card_data.getModule() << ", " << channel_number << " ==> Readout Channel " << data_product_ch_num << std::endl;
	    
	    // here we translate crate/card/daq channel to data product channel number
	    // also need to go from clock time to time stamp
	    opdet::UBOpticalChannelCategory_t ch_category = ub_pmt_channel_map->GetChannelCategory( data_product_ch_num );
	    double window_timestamp = timeService->OpticalClock().Time( time, frame );
            raw::OpDetWaveform rd( window_timestamp, channel_number,win_data_size);
            rd.reserve(win_data_size); // Don't know if this compiles, but it is more efficient. push_back is terrible without it.

	    //std::cout << " into ReadoutCH=" << data_product_ch_num << " category=" << opdet::UBOpChannelEnumName( ch_category ) << std::endl;
	    
	    
            for(ub_RawData::const_iterator it = window_data.begin(); it!= window_data.end(); it++){ 
              rd.push_back(*it & 0xfff);                
            }
            pmtDigitList[ch_category]->emplace_back(rd);
          }
        }//<--End channel_pmt_it for loop
      }//<---End card_pmt_it for loop
    }//<---End seb_pmt_it for loop
    
  }

  // =====================================================================
  void LArRawInputDriverUBooNE::fillBeamData(ubdaq::ub_EventRecord& event_record,
                                             raw::BeamInfo& beamInfo)
  {
    /*
        ubdaq::ub_BeamHeader bh=event_record.getBeamHeader();
    std::vector<ubdaq::ub_BeamData> bdv=event_record.getBeamDataVector();
    if (bdv.size()>0) {
      beamInfo.SetRecordType(bh.getRecordType());
      beamInfo.SetSeconds(bh.getSeconds());
      beamInfo.SetMilliSeconds(bh.getMilliSeconds());
      beamInfo.SetNumberOfDevices(bh.getNumberOfDevices());
      
      for (int i=0;i<bh.getNumberOfDevices();i++) {
        beamInfo.Set(bdv[i].getDeviceName(),bdv[i].getData());
        if (fHistMapBeam.find(bdv[i].getDeviceName())!=fHistMapBeam.end()) 
          fHistMapBeam[bdv[i].getDeviceName()]->Fill(bdv[i].getData()[0]);
      }
    }
    */
  }

  // =====================================================================
  void LArRawInputDriverUBooNE::fillTriggerData(gov::fnal::uboone::datatypes::ub_EventRecord &event_record,
						std::vector<raw::Trigger>& trigInfo)
  {

    ::art::ServiceHandle< util::TimeService > timeService;

    for(auto const& it_trig_map : event_record.getTRIGSEBMap()){

      //int seb_num = it_trig_map.first;
      trig_crate_data_t const& trig_crate = it_trig_map.second;  //  is typedef of ub_Trigger_CrateData_X
      trig_card_data_t const& trig_data  = trig_crate.getTriggerCardData(); // typedef of ub_Trigger_CardData_X
      
      // Make a trigger clock 
      util::ElecClock trig_clock = timeService->TriggerClock( trig_data.getSample(), trig_data.getFrame() );
      double trigger_time = trig_clock.Time();
      double beam_time = 0;
      if ( trig_data.getTriggerData().Trig_Gate1() || trig_data.getTriggerData().Trig_Gate2() ) // 1) NUMI : 2) BNB
	beam_time = trigger_time;
      else {
	std::cerr << "WARNING: THE BEAM TIME VALUE FOR NOT-(BNB or NUMI) TRIGGERS HAS NOT BEEN SETUP!" << std::endl;
      }
      
      raw::Trigger swiz_trig( trig_data.getTrigNumber(),
			      trigger_time,
			      beam_time,
			      (uint32_t)trig_data.getTriggerData().trig_data_1 ); // warning casting 16 bit to 32 bit and praying...
      trigInfo.emplace_back( swiz_trig );
      
    }
  }

  // =====================================================================  
  std::vector<short> LArRawInputDriverUBooNE::decodeChannelTrailer(unsigned short last_adc, unsigned short data)
  {
    // bug fix for missing channel trailer in TPC Data.
    // undoes the hack that fixed the above where the last word is used as the trailer
    // we then use the fake trailer, or frailer, combine it with the last word in the channel data window, or lachadawin, 
    // to recover the end of the channel waveform.

    //std::vector<unsigned short> res;
    std::vector<short> res;
    if(data>>12 == 0x0) {
      //std::cout << "Non-huffman data word..." << std::endl;
      res.push_back(  (short) data & 0xfff);
      return res;
    }
    if(data>>14 == 0x2) {
      //std::cout << "Huffman data word..." << std::endl;
      size_t zero_count=0;
      for(int index=13; index>=0; --index) {
	if(!(data>>index & 0x1)) zero_count +=1;
	else {
	  switch(zero_count){
	      
	  case 0:
	    break;
	  case 1:
	    last_adc -= 1; break;
	  case 2:
	    last_adc += 1; break;
	  case 3:
	    last_adc -= 2; break;
	  case 4:
	    last_adc += 2; break;
	  case 5:
	    last_adc -= 3; break;
	  case 6:
	    last_adc += 3; break;
	  default:

	    std::cerr << "Unexpected 0-count for huffman word: "
		      << "\033[95m"
		      << zero_count << " zeros in the word 0x"
		      << std::hex
		      << data
		      << std::dec
		      << "\033[00m"
		      << std::endl;
	    std::cerr << "Binary representation of the whole word: "
		      << "\033[00m";
	    for(int i=15; i>=0; --i)
	      std::cout << ((data>>i) & 0x1);
	    std::cout << "\033[00m" << std::endl;
	    throw std::exception();
	  }
	  res.push_back((short)last_adc);
	  zero_count = 0;
	}
      }
      return res;
    }

    std::cerr << "\033[93mERROR\033[00m Unexpected upper 4 bit: 0x"
	      << std::hex
	      << ((data >> 12) & 0xf)
	      << std::dec
	      << std::endl;
    throw std::exception();
  }
}//<---Endlris
