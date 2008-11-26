import FWCore.ParameterSet.Config as cms

process = cms.Process("Test")
process.load("Geometry.CMSCommonData.cmsIdealGeometryXML_cfi")

process.load("CondCore.DBCommon.CondDBCommon_cfi")

process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag.globaltag = 'CRAFT_V4P::All'

process.load("CalibTracker.SiStripLorentzAngle.SiStripRandomLorentzAngle_cfi")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)
process.source = cms.Source("EmptySource",
    numberEventsInRun = cms.untracked.uint32(1),
    firstRun = cms.untracked.uint32(1)
)

process.MessageLogger = cms.Service("MessageLogger",
    debug = cms.untracked.PSet(
        threshold = cms.untracked.string('INFO')
    ),
    destinations = cms.untracked.vstring('LA_Craft_Uniform')
)

process.TrackerGeometricDetESModule = cms.ESProducer("TrackerGeometricDetESModule",
    fromDDD = cms.bool(True)
)

#process.TrackerDigiGeometryESModule = cms.ESProducer("TrackerDigiGeometryESModule")

process.Timing = cms.Service("Timing")

process.CondDBCommon.connect = 'sqlite_file:LA_CRAFT_UNIFORM.db'

process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    connect = cms.string('sqlite_file:LA_CRAFT_UNIFORM.db'),
    timetype = cms.untracked.string('runnumber'),
    BlobStreamerName = cms.untracked.string('TBufferBlobStreamingService'),
    DBParameters = cms.PSet(
        authenticationPath = cms.untracked.string('/afs/cern.ch/cms/DB/conddb')
    ),
    toPut = cms.VPSet(cms.PSet(
	record = cms.string('SiStripLorentzAngleRcd'),
        tag = cms.string('SiStripLA_CRAFT_V2P-V3P_220_Uniform')
    ))
)

process.p = cms.Path(process.sistripLorentzAngle)



