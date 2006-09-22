
#include <memory>
#include <string>
#include <iostream>
#include <TMath.h>
#include "CalibTracker/SiStripLorentzAngle/test/AnalyzeTracks.h"

#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "Geometry/Vector/interface/GlobalPoint.h"
#include "Geometry/Vector/interface/GlobalVector.h"
#include "Geometry/Vector/interface/LocalVector.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/CommonDetUnit/interface/GeomDetType.h"
#include "Geometry/CommonDetUnit/interface/GeomDetUnit.h"
#include "DataFormats/TrackingRecHit/interface/TrackingRecHit.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackExtra.h"
#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"
#include "DataFormats/SiStripDetId/interface/TIBDetId.h"
#include "DataFormats/SiStripDetId/interface/TOBDetId.h"
#include "DataFormats/TrackerRecHit2D/interface/SiStripMatchedRecHit2D.h"
#include "TrackingTools/Records/interface/TransientRecHitRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/GluedGeomDet.h"

using namespace std;
AnalyzeTracks::AnalyzeTracks(edm::ParameterSet const& conf) : 
  conf_(conf)
{
  anglefinder_=new  TrackLocalAngle(conf);
}

// Virtual destructor needed.
AnalyzeTracks::~AnalyzeTracks() {  
  delete anglefinder_;
}  

// Functions that gets called by framework every event
void AnalyzeTracks::analyze(const edm::Event& e, const edm::EventSetup& es)
{
  anglefinder_->init(e,es);
  if(!(conf_.getParameter<bool>("MTCCtrack"))){
    std::string src=conf_.getParameter<std::string>( "src" );
    edm::Handle<reco::TrackCollection> trackCollection;
    e.getByLabel(src, trackCollection);
    const reco::TrackCollection *tracks=trackCollection.product();
    reco::TrackCollection::const_iterator tciter;
    if(tracks->size()>0){
      for(tciter=tracks->begin();tciter!=tracks->end();tciter++){
	std::vector<std::pair<const TrackingRecHit *,float> > hitangle=anglefinder_->findtrackangle(*tciter);
	std::vector<std::pair<const TrackingRecHit *,float> >::iterator iter;
	for(iter=hitangle.begin();iter!=hitangle.end();iter++){
	  LocalPoint position= (iter->first)->localPosition();
	  float angle=iter->second;
	  edm::LogInfo("AnalyzeTracks")<<"Local Position= "<<position;
	  edm::LogInfo("AnalyzeTracks")<<"Local Angle= "<<angle;
	}
      }
    }
    else edm::LogInfo("AnalyzeTracks")<<"No track found in the event";
  }
  else{
    edm::LogInfo("AnalyzeTracks")<<"Analyze MTCC track ";

    edm::Handle<TrajectorySeedCollection> seedcoll;
    e.getByType(seedcoll);
    LogDebug("AnalyzeTracks::analyze")<<"Getting used rechit";
    edm::Handle<reco::TrackCollection> trackCollection;
    e.getByType(trackCollection);
    const reco::TrackCollection *tracks=trackCollection.product();
    
    std::vector<std::pair<const TrackingRecHit *,float> >hitangle =anglefinder_->findtrackangle((*(*seedcoll).begin()),tracks->front());
    edm::LogInfo("AnalyzeTracks")<<"Number of hits= "<<hitangle.size();
    vector<std::pair<const TrackingRecHit * ,float> >::iterator iter;
    for(iter=hitangle.begin();iter!=hitangle.end();iter++){
      float angle=iter->second;
      edm::LogInfo("AnalyzeTracks")<<"Local Angle= "<<angle;
      LocalPoint position= (iter->first)->localPosition();
      edm::LogInfo("AnalyzeTracks")<<"Local Position= "<<position;
    }
  }
}
