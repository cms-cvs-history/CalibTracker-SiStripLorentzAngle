#include "CondFormats/SiStripObjects/interface/SiStripLorentzAngle.h"
#include "CondFormats/DataRecord/interface/SiStripLorentzAngleRcd.h"

#include "CalibTracker/SiStripLorentzAngle/test/SiStripLorentzAngleReader.h"

#include <iostream>
#include <stdio.h>
#include <sys/time.h>


using namespace cms;

SiStripLorentzAngleReader::SiStripLorentzAngleReader( const edm::ParameterSet& iConfig ):
  printdebug_(iConfig.getUntrackedParameter<bool>("printDebug",false)){}

SiStripLorentzAngleReader::~SiStripLorentzAngleReader(){}

void SiStripLorentzAngleReader::analyze( const edm::Event& e, const edm::EventSetup& iSetup){
  
  edm::ESHandle<SiStripLorentzAngle> SiStripLorentzAngle_;
  iSetup.get<SiStripLorentzAngleRcd>().get(SiStripLorentzAngle_);
  edm::LogInfo("SiStripLorentzAngleReader") << "[SiStripLorentzAngleReader::analyze] End Reading SiStripLorentzAngle" << std::endl;
  
  std::map<unsigned int,float> detid_la= SiStripLorentzAngle_->getLorentzAngles();
  std::map<unsigned int,float>::const_iterator it;
  for (it=detid_la.begin();it!=detid_la.end();it++)
      {
	edm::LogInfo("SiStripLorentzAngleReader")  << "detid " << it->first << " \t"
						   << " Lorentz angle  " << it->second;
      } 
}

