import FWCore.ParameterSet.Config as cms

process = cms.Process("Test")
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
    destinations = cms.untracked.vstring('LA_Craft_Uniform_Reader')
)

process.Timing = cms.Service("Timing")

process.PoolDBESSource = cms.ESSource("PoolDBESSource",
    BlobStreamerName = cms.untracked.string('TBufferBlobStreamingService'),
    DBParameters = cms.PSet(
        messageLevel = cms.untracked.int32(2),
        authenticationPath = cms.untracked.string('/afs/cern.ch/cms/DB/conddb')
    ),
    timetype = cms.string('runnumber'),
    toGet = cms.VPSet(cms.PSet(
        record = cms.string('SiStripLorentzAngleRcd'),
        tag = cms.string('SiStripLA_CRAFT_V2P-V3P_220_Uniform')
    )),
    connect = cms.string('sqlite_file:LA_CRAFT_UNIFORM.db')
)

process.prod = cms.EDFilter("SiStripLorentzAngleReader")

process.p = cms.Path(process.prod)


