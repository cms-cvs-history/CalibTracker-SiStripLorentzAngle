#include <string>
#include <iostream>
#include <fstream>
#include <functional>


#include "CalibTracker/SiStripLorentzAngle/interface/SiStripLAProfileBooker.h"
#include "CalibTracker/Records/interface/SiStripDetCablingRcd.h"
#include "CalibFormats/SiStripObjects/interface/SiStripDetCabling.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/CommonDetUnit/interface/GeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/GluedGeomDet.h"
#include "Geometry/CommonTopologies/interface/StripTopology.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"
#include "DataFormats/SiStripDetId/interface/TIBDetId.h"
#include "DataFormats/SiStripDetId/interface/TIDDetId.h"
#include "DataFormats/SiStripDetId/interface/TOBDetId.h"
#include "DataFormats/SiStripDetId/interface/TECDetId.h"
#include "DataFormats/TrackerRecHit2D/interface/SiStripMatchedRecHit2D.h"
#include "DataFormats/TrackerRecHit2D/interface/ProjectedSiStripRecHit2D.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"

#include "DQM/SiStripCommon/interface/ExtractTObject.h"
#include "DQM/SiStripCommon/interface/SiStripHistoId.h"
#include "DQM/SiStripCommon/interface/SiStripFolderOrganizer.h"

#include "TrackingTools/TransientTrackingRecHit/interface/TransientTrackingRecHit.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"

#include <TProfile.h>
#include <TF1.h>

 
class DetIdLess 
  : public std::binary_function< const SiStripRecHit2D*,const SiStripRecHit2D*,bool> {
public:
  
  DetIdLess() {}
  
  bool operator()( const SiStripRecHit2D* a, const SiStripRecHit2D* b) const {
    return *(a->cluster())<*(b->cluster());
  }
};
  

  //Constructor

SiStripLAProfileBooker::SiStripLAProfileBooker(edm::ParameterSet const& conf) : 
  conf_(conf)
{
}

  //BeginJob

void SiStripLAProfileBooker::beginJob(const edm::EventSetup& c){

 
  //get magnetic field and geometry from ES
  edm::ESHandle<MagneticField> esmagfield;
  c.get<IdealMagneticFieldRecord>().get(esmagfield);
  const MagneticField *  magfield=&(*esmagfield);
  
  edm::ESHandle<TrackerGeometry> estracker;
  c.get<TrackerDigiGeometryRecord>().get(estracker);
  tracker=&(*estracker); 

  std::vector<uint32_t> activeDets;
  edm::ESHandle<SiStripDetCabling> tkmechstruct=0;
  if (conf_.getParameter<bool>("UseStripCablingDB")){ 
    c.get<SiStripDetCablingRcd>().get(tkmechstruct);
    activeDets.clear();
    tkmechstruct->addActiveDetectorsRawIds(activeDets);
  }
  else {
    const TrackerGeometry::DetIdContainer& Id = estracker->detIds();
    TrackerGeometry::DetIdContainer::const_iterator Iditer;    
    activeDets.clear();
    for(Iditer=Id.begin();Iditer!=Id.end();Iditer++){
      activeDets.push_back(Iditer->rawId());
    }
  }

  edm::InputTag TkTag = conf_.getParameter<edm::InputTag>("Tracks");
  //Get Ids;
  double ModuleRangeMin=conf_.getParameter<double>("ModuleXMin");
  double ModuleRangeMax=conf_.getParameter<double>("ModuleXMax");
  double TIBRangeMin=conf_.getParameter<double>("TIBXMin");
  double TIBRangeMax=conf_.getParameter<double>("TIBXMax");
  double TOBRangeMin=conf_.getParameter<double>("TOBXMin");
  double TOBRangeMax=conf_.getParameter<double>("TOBXMax");
  
  // use SistripHistoId for producing histogram id (and title)
  SiStripHistoId hidmanager;

  // create SiStripFolderOrganizer
  SiStripFolderOrganizer folder_organizer;

  dbe_ = edm::Service<DQMStore>().operator->();

  //get all detids

  for(std::vector<uint32_t>::const_iterator Id = activeDets.begin(); Id!=activeDets.end(); Id++){

    //  for(Iditer=Id.begin();Iditer!=Id.end();Iditer++){ //loop on detids
    DetId Iditero=DetId(*Id);
    DetId *Iditer=&Iditero;
    if((Iditer->subdetId() == int(StripSubdetector::TIB)) || (Iditer->subdetId() == int(StripSubdetector::TOB))){ //include only barrel

      // create a TProfile for each module
      StripSubdetector subid(*Iditer);
      std::string hid;
      //Mono single sided detectors
      LocalPoint p;
      const GeomDetUnit * stripdet=dynamic_cast<const GeomDetUnit*>(tracker->idToDet(subid));
      if(stripdet==0)continue;
      const StripTopology& topol=(StripTopology&)stripdet->topology();
      float thickness=stripdet->specificSurface().bounds().thickness();
		
      folder_organizer.setDetectorFolder(Iditer->rawId());
      hid = hidmanager.createHistoId(TkTag.label().c_str(),"det",Iditer->rawId());
      MonitorElement * profile=dbe_->bookProfile(hid,hid,30,ModuleRangeMin,ModuleRangeMax,20,0,5,"");
      detparameters *param=new detparameters;
      histos[Iditer->rawId()] = profile;
      detmap[Iditer->rawId()] = param;
      param->thickness = thickness*10000;
      param->pitch = topol.localPitch(p)*10000;


      const GlobalPoint globalp = (stripdet->surface()).toGlobal(p);
      GlobalVector globalmagdir = magfield->inTesla(globalp);
      param->magfield=(stripdet->surface()).toLocal(globalmagdir);

      profile->setAxisTitle("tan(#theta_{t})",1);
      profile->setAxisTitle("Cluster size",2);

      // create a summary histo if it does not exist already
      std::string name;
      unsigned int layerid;
      getlayer(subid,name,layerid);
      name+=TkTag.label().c_str();
      if(summaryhisto.find(layerid)==(summaryhisto.end())){
	folder_organizer.setSiStripFolder();
	MonitorElement * summaryprofile=0;
	if (subid.subdetId()==int (StripSubdetector::TIB)||subid.subdetId()==int (StripSubdetector::TID))
	  summaryprofile=dbe_->bookProfile(name,name,30,TIBRangeMin,TIBRangeMax,20,0,5,"");
	else if (subid.subdetId()==int (StripSubdetector::TOB)||subid.subdetId()==int (StripSubdetector::TEC))
	  summaryprofile=dbe_->bookProfile(name,name,30,TOBRangeMin,TOBRangeMax,20,0,5,"");
	if(summaryprofile){
	  detparameters *summaryparam=new detparameters;
	  summaryhisto[layerid] = summaryprofile;
	  summarydetmap[layerid] = summaryparam;
	  summaryparam->thickness = thickness*10000;
	  summaryparam->pitch = topol.localPitch(p)*10000;
	  summaryprofile->setAxisTitle("tan(#theta_{t})",1);
	  summaryprofile->setAxisTitle("Cluster size",2);
	}
      }
    } 
  } 

}

SiStripLAProfileBooker::~SiStripLAProfileBooker() {  
  detparmap::iterator detpariter;
  for( detpariter=detmap.begin(); detpariter!=detmap.end();++detpariter)delete detpariter->second;
  for( detpariter=summarydetmap.begin(); detpariter!=summarydetmap.end();++detpariter)delete detpariter->second;
  fitmap::iterator  fitpar;
  for( fitpar=summaryfits.begin(); fitpar!=summaryfits.end();++fitpar)delete fitpar->second;

}  

// Analyzer: Functions that gets called by framework every event

void SiStripLAProfileBooker::analyze(const edm::Event& e, const edm::EventSetup& es)
{
  
  //Analysis of Trajectory-RecHits
        
  edm::InputTag TkTag = conf_.getParameter<edm::InputTag>("Tracks");
  int nHitMin = conf_.getParameter<int>("NHitMin");
  
  edm::Handle<reco::TrackCollection> trackCollection;
  e.getByLabel(TkTag,trackCollection);
    
  edm::Handle<std::vector<Trajectory> > TrajectoryCollection;
  e.getByLabel(TkTag,TrajectoryCollection);
  
  const reco::TrackCollection *tracks=trackCollection.product();
 
  std::map<const SiStripRecHit2D*,std::pair<float,float>,DetIdLess> hitangleassociation;
  
  int trackcollsize=tracks->size();
  int trajsize=TrajectoryCollection->size();
  
  edm::LogInfo("SiStripLAProfileBooker::analyze") <<" Number of tracks in event = "<<trackcollsize<<"\n";
  edm::LogInfo("SiStripLAProfileBooker::analyze") <<" Number of trajectories in event = "<<trajsize<<"\n";

  std::vector<Trajectory>::const_iterator theTraj;
  for(theTraj = TrajectoryCollection->begin(); theTraj!= TrajectoryCollection->end();theTraj++){ //loop on trajectories
    if(theTraj->foundHits()>=nHitMin){
      std::vector<TrajectoryMeasurement> TMeas=theTraj->measurements();
      std::vector<TrajectoryMeasurement>::iterator itm;
      
      for (itm=TMeas.begin();itm!=TMeas.end();itm++){ //loop on hits
	TrajectoryStateOnSurface tsos=itm->updatedState();
	const TransientTrackingRecHit::ConstRecHitPointer thit=itm->recHit();
	if((thit->geographicalId().subdetId() == int(StripSubdetector::TIB)) ||  thit->geographicalId().subdetId()== int(StripSubdetector::TOB)){ //include only barrel
	  const SiStripMatchedRecHit2D* matchedhit=dynamic_cast<const SiStripMatchedRecHit2D*>((*thit).hit());
	  const ProjectedSiStripRecHit2D* phit=dynamic_cast<const ProjectedSiStripRecHit2D*>((*thit).hit());
	  const SiStripRecHit2D* hit=dynamic_cast<const SiStripRecHit2D*>((*thit).hit());
	  if(phit) hit=&(phit->originalHit());
	  LocalVector trackdirection=tsos.localDirection();
	  
	  if(matchedhit){//if matched hit...
	    
	    GluedGeomDet * gdet=(GluedGeomDet *)tracker->idToDet(matchedhit->geographicalId());
	    
	    GlobalVector gtrkdir=gdet->toGlobal(trackdirection);	
	    
	    // THIS THE POINTER TO THE MONO HIT OF A MATCHED HIT 
	    const SiStripRecHit2D *monohit=matchedhit->monoHit();
	    
	    const edm::Ref<edm::DetSetVector<SiStripCluster>, SiStripCluster, edm::refhelper::FindForDetSetVector<SiStripCluster> > monocluster=monohit->cluster();
	    const GeomDetUnit * monodet=gdet->monoDet();
	    
	    LocalVector monotkdir=monodet->toLocal(gtrkdir);
	    //size=(monocluster->amplitudes()).size();
	    if(monotkdir.z()!=0){
	      
	      // THE LOCAL ANGLE (MONO)
	      float tanangle = monotkdir.x()/monotkdir.z();
	      std::map<const SiStripRecHit2D *,std::pair<float,float>,DetIdLess>::iterator alreadystored=hitangleassociation.find(monohit);
	      if(alreadystored != hitangleassociation.end()){//decide which hit take
		if(itm->estimate() <  alreadystored->second.first) hitangleassociation.insert(std::make_pair(monohit, std::make_pair(itm->estimate(),tanangle)));
	      }
	      else hitangleassociation.insert(make_pair(monohit, std::make_pair(itm->estimate(),tanangle))); 
	      
	      //cluster and trackdirection on stereo det
	      
	      // THIS THE POINTER TO THE STEREO HIT OF A MATCHED HIT 
	      const SiStripRecHit2D *stereohit=matchedhit->stereoHit();
	      const edm::Ref<edm::DetSetVector<SiStripCluster>, SiStripCluster, edm::refhelper::FindForDetSetVector<SiStripCluster> > stereocluster=stereohit->cluster();
	      const GeomDetUnit * stereodet=gdet->stereoDet(); 
	      LocalVector stereotkdir=stereodet->toLocal(gtrkdir);
	      
	      if(stereotkdir.z()!=0){
		
		// THE LOCAL ANGLE (STEREO)
		float tanangle = stereotkdir.x()/stereotkdir.z();
		std::map<const SiStripRecHit2D *,std::pair<float,float>,DetIdLess>::iterator alreadystored=hitangleassociation.find(stereohit);
		if(alreadystored != hitangleassociation.end()){//decide which hit take
		  if(itm->estimate() <  alreadystored->second.first) hitangleassociation.insert(std::make_pair(stereohit, std::make_pair(itm->estimate(),tanangle)));
		}
		else hitangleassociation.insert(std::make_pair(stereohit, std::make_pair(itm->estimate(),tanangle))); 		  
	      }
	    }
	  }
	  else if(hit){
	    //  hit= POINTER TO THE RECHIT
	    const edm::Ref<edm::DetSetVector<SiStripCluster>, SiStripCluster, edm::refhelper::FindForDetSetVector<SiStripCluster> > cluster=hit->cluster();
	    
	    if(trackdirection.z()!=0){
	    
	      // THE LOCAL ANGLE (STEREO)
	      float tanangle = trackdirection.x()/trackdirection.z();
	      std::map<const SiStripRecHit2D *,std::pair<float,float>, DetIdLess>::iterator alreadystored=hitangleassociation.find(hit);
	      if(alreadystored != hitangleassociation.end()){//decide which hit take
		if(itm->estimate() <  alreadystored->second.first) hitangleassociation.insert(std::make_pair(hit, std::make_pair(itm->estimate(),tanangle)));
	    }
	      else hitangleassociation.insert(std::make_pair(hit,std::make_pair(itm->estimate(), tanangle) ) ); 
	    }
	  }
	}
      }
    }
  }
    std::map<const SiStripRecHit2D *,std::pair<float,float>,DetIdLess>::iterator hitsiter;
    
    for(hitsiter=hitangleassociation.begin();hitsiter!=hitangleassociation.end();hitsiter++){
    
    const SiStripRecHit2D* hit=hitsiter->first;
    const edm::Ref<edm::DetSetVector<SiStripCluster>, SiStripCluster, edm::refhelper::FindForDetSetVector<SiStripCluster> > cluster=hit->cluster();

    int size=(cluster->amplitudes()).size();
    
	
    StripSubdetector detid=(StripSubdetector)hit->geographicalId();
	  
    float tangent = hitsiter->second.second;
	  
    //Sign and XZ plane projection correction applied in TrackLocalAngle (TIB|TOB layers)
      
    detparmap::iterator thedet=detmap.find(detid.rawId());
    LocalVector localmagdir;
    if(thedet!=detmap.end())localmagdir=thedet->second->magfield;
    float localmagfield = localmagdir.mag();
    
    if(localmagfield != 0.){
      
      LocalVector ylocal(0,1,0);
      
      float normprojection = (localmagdir * ylocal)/(localmagfield);
      
      if(normprojection == 0.)LogDebug("SiStripLAProfileBooker::analyze")<<"Error: YBprojection = 0";
      
      else{
	float signprojcorrection = 1/normprojection;
	tangent*=signprojcorrection;
      }
    }
	  
    //Filling histograms
    histomap::iterator thehisto=histos.find(detid.rawId());

    if(thehisto==histos.end())edm::LogError("SiStripLAProfileBooker::analyze")<<"Error: the profile associated to"<<detid.rawId()<<"does not exist! ";
    else thehisto->second->Fill(tangent,size);
    //Summary histograms
    std::string name;
    unsigned int layerid;
    getlayer(detid,name,layerid);
    histomap::iterator thesummaryhisto=summaryhisto.find(layerid);
    if(thesummaryhisto==summaryhisto.end())edm::LogError("SiStripLAProfileBooker::analyze")<<"Error: the profile associated to subdet "<<name<<"does not exist! ";
    else thesummaryhisto->second->Fill(tangent,size);

  }
}

 
//Makename function
 
void SiStripLAProfileBooker::getlayer(const DetId & detid, std::string &name,unsigned int &layerid){
    int layer;
    std::stringstream layernum;

    if(detid.subdetId() == int (StripSubdetector::TIB)){
      TIBDetId TIBid=TIBDetId(detid);
      name+="TIB_Layer_";
      layer = TIBid.layer();
    }

    else if(detid.subdetId() == int (StripSubdetector::TID)){
      TIDDetId TIDid=TIDDetId(detid);
      name+="TID_Ring_";
      layer = TIDid.ring();
    }

    else if(detid.subdetId() == int (StripSubdetector::TOB)){
      TOBDetId TOBid=TOBDetId(detid);
      name+="TOB_Layer_";
      layer = TOBid.layer();

    }

    else if(detid.subdetId() == int (StripSubdetector::TEC)){
      TECDetId TECid=TECDetId(detid);
      name+="TEC_Ring_";
      layer = TECid.ring();
    }
    layernum<<layer;
    name+=layernum.str();
    layerid=detid.subdetId()*10+layer;
  
}
 
void SiStripLAProfileBooker::endJob(){
  fitmap fits;

  //Histograms fit
  TF1 *fitfunc=0;
  double ModuleRangeMin=conf_.getParameter<double>("ModuleFitXMin");
  double ModuleRangeMax=conf_.getParameter<double>("ModuleFitXMax");
  double TIBRangeMin=conf_.getParameter<double>("TIBFitXMin");
  double TIBRangeMax=conf_.getParameter<double>("TIBFitXMax");
  double TOBRangeMin=conf_.getParameter<double>("TOBFitXMin");
  double TOBRangeMax=conf_.getParameter<double>("TOBFitXMax");
  
  histomap::iterator hist_it;
  fitfunc= new TF1("fitfunc","([4]/[3])*[1]*(TMath::Abs(x-[0]))+[2]",-1,1);
  
  for(hist_it=histos.begin();hist_it!=histos.end(); hist_it++){
    if(hist_it->second->getEntries()>100){
      float thickness=0,pitch=-1;
      detparmap::iterator detparit=detmap.find(hist_it->first);
      if(detparit!=detmap.end()){
	thickness = detparit->second->thickness;
	pitch = detparit->second->pitch;
      }
      
      fitfunc->SetParameter(0, 0);
      fitfunc->SetParameter(1, 0);
      fitfunc->SetParameter(2, 1);
      fitfunc->FixParameter(3, pitch);
      fitfunc->FixParameter(4, thickness);
      int fitresult=-1;
      TProfile* theProfile=ExtractTObject<TProfile>().extract(hist_it->second);
      fitresult=theProfile->Fit(fitfunc,"N","",ModuleRangeMin, ModuleRangeMax);
      detparmap::iterator thedet=detmap.find(hist_it->first);
      LocalVector localmagdir;
      if(thedet!=detmap.end())localmagdir=thedet->second->magfield;
      float localmagfield = localmagdir.mag();

      histofit *fit= new histofit;
      fits[hist_it->first] =fit;
      
      fit->chi2 = fitfunc->GetChisquare();
      fit->ndf  = fitfunc->GetNDF();
      fit->p0   = fitfunc->GetParameter(0)/localmagfield;
      fit->p1   = fitfunc->GetParameter(1);
      fit->p2   = fitfunc->GetParameter(2);
      fit->errp0   = fitfunc->GetParError(0)/localmagfield;
      fit->errp1   = fitfunc->GetParError(1);
      fit->errp2   = fitfunc->GetParError(2);
    }
  }
  
  histomap::iterator summaryhist_it;
  
  for(summaryhist_it=summaryhisto.begin();summaryhist_it!=summaryhisto.end(); summaryhist_it++){
    if(summaryhist_it->second->getEntries()>100){
      float thickness=0,pitch=-1;
      detparmap::iterator detparit=summarydetmap.find(summaryhist_it->first);
      if(detparit!=summarydetmap.end()){
	thickness = detparit->second->thickness;
	pitch = detparit->second->pitch;
      }
      
      fitfunc->SetParameter(0, 0);
      fitfunc->SetParameter(1, 0);
      fitfunc->SetParameter(2, 1);
      fitfunc->FixParameter(3, pitch);
      fitfunc->FixParameter(4, thickness);
      int fitresult=-1;
      TProfile* thesummaryProfile=ExtractTObject<TProfile>().extract(summaryhist_it->second);
      if ((summaryhist_it->first)/10==int (StripSubdetector::TIB)||(summaryhist_it->first)/10==int (StripSubdetector::TID))
	fitresult=thesummaryProfile->Fit(fitfunc,"N","",TIBRangeMin, TIBRangeMax);
      else if ((summaryhist_it->first)/10==int (StripSubdetector::TOB)||(summaryhist_it->first)/10==int (StripSubdetector::TEC))
	fitresult=thesummaryProfile->Fit(fitfunc,"N","",TOBRangeMin, TOBRangeMax);
      //if(fitresult==0){
	histofit * summaryfit=new histofit;
	summaryfits[summaryhist_it->first] = summaryfit;
	summaryfit->chi2 = fitfunc->GetChisquare();
	summaryfit->ndf  = fitfunc->GetNDF();
	summaryfit->p0   = fitfunc->GetParameter(0);
	summaryfit->p1   = fitfunc->GetParameter(1);
	summaryfit->p2   = fitfunc->GetParameter(2);
	summaryfit->errp0   = fitfunc->GetParError(0);
	summaryfit->errp1   = fitfunc->GetParError(1);
	summaryfit->errp2   = fitfunc->GetParError(2);
	// }
    }
  }
  delete fitfunc;
  
  //File with fit parameters  
  
  std::string fitName=conf_.getParameter<std::string>("fitName");
  fitName+=".txt";
  
  ofstream fit;
  fit.open(fitName.c_str());
  
  //  fit<<">>> ANALYZED RUNS = ";
  //for(int n=0;n!=runcounter;n++){
    //fit<<runvector[n]<<", ";}
  //fit<<endl;
  
  //fit<<">>> TOTAL EVENTS = "<<eventcounter<<std::endl;
  //  fit<<">>> NUMBER OF TRACKS = "<<trackcounter<<endl<<std::endl;
  //fit<<">>> NUMBER OF RECHITS = "<<hitcounter<<endl<<std::endl;
  
  fit<<">>> NUMBER OF DETECTOR HISTOGRAMS = "<<histos.size()<<std::endl;
     
  std::string subdetector;
  for(summaryhist_it=summaryhisto.begin();summaryhist_it!=summaryhisto.end(); summaryhist_it++){
    if ((summaryhist_it->first)/10==int (StripSubdetector::TIB))subdetector="TIB";
    else if ((summaryhist_it->first)/10==int (StripSubdetector::TID))subdetector="TID";
    else if ((summaryhist_it->first)/10==int (StripSubdetector::TOB))subdetector="TOB";
    else if ((summaryhist_it->first)/10==int (StripSubdetector::TEC))subdetector="TEC";
    float thickness=0,pitch=-1;
    detparmap::iterator detparit=summarydetmap.find(summaryhist_it->first);
    if(detparit!=summarydetmap.end()){
      thickness = detparit->second->thickness;
      pitch = detparit->second->pitch;
    }    
    fitmap::iterator  fitpar=summaryfits.find(summaryhist_it->first);
    
    fit<<std::endl<<"--------------------------- SUMMARY FIT: "<<subdetector<<" LAYER/RING "<<(summaryhist_it->first%10)<<" -------------------------"<<std::endl<<std::endl;
    fit<<"Number of entries = "<<summaryhist_it->second->getEntries()<<std::endl<<std::endl;
    fit<<"Detector thickness = "<<thickness<<" um "<<std::endl;
    fit<<"Detector pitch = "<<pitch<<" um "<<std::endl<<std::endl;    
    if(fitpar!=summaryfits.end()){
      fit<<"Chi Square/ndf = "<<(fitpar->second->chi2)/(fitpar->second->ndf)<<std::endl;
      fit<<"NdF        = "<<fitpar->second->ndf<<std::endl;
      fit<<"p0 = "<<fitpar->second->p0<<"     err p0 = "<<fitpar->second->errp0<<std::endl;
      fit<<"p1 = "<<fitpar->second->p1<<"     err p1 = "<<fitpar->second->errp1<<std::endl;
      fit<<"p2 = "<<fitpar->second->p2<<"     err p2 = "<<fitpar->second->errp2<<std::endl<<std::endl;
    }
    else fit<<"no fit parameters available"<<std::endl;
  }
  
  for(hist_it=histos.begin();hist_it!=histos.end(); hist_it++){   
    float thickness=0,pitch=-1;
    detparmap::iterator detparit=detmap.find(hist_it->first);
    if(detparit!=detmap.end()){
      thickness = detparit->second->thickness;
      pitch = detparit->second->pitch;
    }    
    fitmap::iterator  fitpar=fits.find(hist_it->first);
    if(hist_it->second->getEntries()>0){
      fit<<std::endl<<"-------------------------- MODULE HISTOGRAM FIT ------------------------"<<std::endl<<std::endl;
      DetId id= DetId(hist_it->first);
      fit<<"Module id= "<<id.rawId()<<std::endl<<std::endl;
      fit<<"Number of entries = "<<hist_it->second->getEntries()<<std::endl<<std::endl;
      fit<<"Detector thickness = "<<thickness<<" um "<<std::endl;
      fit<<"Detector pitch = "<<pitch<<" um "<<std::endl<<std::endl;
      if(fitpar!=fits.end()){
	fit<<"Chi Square/ndf = "<<(fitpar->second->chi2)/(fitpar->second->ndf)<<std::endl;
	fit<<"NdF        = "<<fitpar->second->ndf<<std::endl;
	fit<<"p0 = "<<fitpar->second->p0<<"     err p0 = "<<fitpar->second->errp0<<std::endl;
	fit<<"p1 = "<<fitpar->second->p1<<"     err p1 = "<<fitpar->second->errp1<<std::endl;
	fit<<"p2 = "<<fitpar->second->p2<<"     err p2 = "<<fitpar->second->errp2<<std::endl<<std::endl;
      }    
    }
  }
  fit.close(); 
  std::string outputFile_ =conf_.getUntrackedParameter<std::string>("fileName", "LorentzAngle.root");
  dbe_->save(outputFile_);
}
