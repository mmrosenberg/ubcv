#ifndef __FMWKINTERFACE_CXX__
#define __FMWKINTERFACE_CXX__

#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "Base/larbys.h"
#include "FMWKInterface.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesServiceStandard.h"
#include "lardata/DetectorInfoServices/LArPropertiesServiceStandard.h"
#include "lardata/DetectorInfoServices/DetectorClocksServiceStandard.h"
#include "larevt/SpaceChargeServices/SpaceChargeService.h"

namespace supera {

  ::geo::WireID ChannelToWireID(unsigned int ch)
  { 
      auto const* geom = ::lar::providerFrom<geo::Geometry>();
      return geom->ChannelToWire(ch).front();
  }
  
  double DriftVelocity(detinfo::DetectorPropertiesData const& detProp)
  { 
    return detProp.DriftVelocity();
  }
  
  unsigned int Nchannels()
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->Nchannels();
  }
  
  unsigned int Nplanes()
  { 
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->Nplanes();
  }
  
  unsigned int Nwires(unsigned int plane)
  { 
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->Nwires(plane); 
  }
  
  unsigned int NearestWire(const TVector3& xyz, unsigned int plane)
  {
    double min_wire=0;
    double max_wire=Nwires(plane)-1;
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    
    double wire = geom->WireCoordinate(xyz[1],xyz[2],plane,0,0) + 0.5;
    if(wire<min_wire) wire = min_wire;
    if(wire>max_wire) wire = max_wire;
    
    return (unsigned int)wire;
  }

  unsigned int NearestWire(const double* xyz, unsigned int plane)
  {
    double min_wire=0;
    double max_wire=Nwires(plane)-1;
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    
    double wire = geom->WireCoordinate(xyz[1],xyz[2],plane,0,0) + 0.5;
    if(wire<min_wire) wire = min_wire;
    if(wire>max_wire) wire = max_wire;
    
    return (unsigned int)wire;
  }

  double WireAngleToVertical(unsigned int plane)
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->WireAngleToVertical(geo::View_t(plane));
  }

  double WirePitch(size_t plane)
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->WirePitch(plane);
  }

  double DetHalfWidth() 
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->DetHalfWidth();
  }

  double DetHalfHeight() 
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->DetHalfHeight();
  }

  double DetLength() 
  {
    auto const* geom = ::lar::providerFrom<geo::Geometry>();
    return geom->DetLength();
  }
  
  int TPCG4Time2Tick(detinfo::DetectorClocksData const& clockData, double ns)
  { 
    return clockData.TPCG4Time2Tick(ns);
  }

  int TPCG4Time2TDC(detinfo::DetectorClocksData const& clockData, double ns)
  {
    return clockData.TPCG4Time2TDC(ns);
  }
  
  double TPCTDC2Tick(detinfo::DetectorClocksData const& clockData, double tdc)
  { 
    return clockData.TPCTDC2Tick(tdc);
  }

  double TPCTickPeriod(detinfo::DetectorClocksData const& clockData)
  {
    return clockData.TPCClock().TickPeriod();
  }

  double TriggerOffsetTPC(detinfo::DetectorClocksData const& clockData)
  {
    return clockData.TriggerOffsetTPC();
  }
  
  double PlaneTickOffset(detinfo::DetectorClocksData const& clockData,
                         detinfo::DetectorPropertiesData const& detProp,
                         size_t plane0, size_t plane1)
  {
    static double pitch = ::lar::providerFrom<geo::Geometry>()->PlanePitch();
    double tick_period = clockData.TPCClock().TickPeriod();
    return (plane1 - plane0) * pitch / DriftVelocity(detProp) / tick_period;
  }

  void ApplySCE(double& x, double& y, double& z)
  {
    auto xyz = ::lar::providerFrom<spacecharge::SpaceChargeService>()->GetPosOffsets(geo::Point_t(x,y,z));
    x = x - xyz.X() + 0.7;
    y = y + xyz.Y();
    z = z + xyz.Z();
  }

  void ApplySCE(double* xyz)
  {
    double x = xyz[0];
    double y = xyz[1];
    double z = xyz[2];
    ApplySCE(x,y,z);
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
  }

  void ApplySCE(TVector3& xyz)
  {
    double x = xyz[0];
    double y = xyz[1];
    double z = xyz[2];
    ApplySCE(x,y,z);
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
  }

}

#endif
