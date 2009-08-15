import FWCore.ParameterSet.Config as cms

process = cms.Process('CALIB')
process.load('CalibTracker.SiStripLorentzAngle.LA_Tree_RECO_cff')
process.GlobalTag.globaltag = 'GR09_P_V1::All'

process.add_( cms.Service( "TFileService",
                           fileName = cms.string( 'calibTree.root' ),
                           closeFileFast = cms.untracked.bool(True)  ) )

#following ignored by CRAB
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1000) )
process.source = cms.Source (
    "PoolSource",
    fileNames = cms.untracked.vstring('/store/express/CRAFT09/ExpressMuon/FEVT/v1/000/110/998/FEBB5B99-C389-DE11-9480-000423D98804.root'),
    secondaryFileNames = cms.untracked.vstring())

