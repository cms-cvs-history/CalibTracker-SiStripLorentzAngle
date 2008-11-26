import FWCore.ParameterSet.Config as cms

process = cms.Process("analyze")
process.load("Configuration.StandardSequences.MagneticField_cff")

process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag.globaltag = 'CRAFT_V4P::All'
#process.GlobalTag.globaltag = 'CRUZETALL_V1::All'

#process.load("Configuration.StandardSequences.FakeConditions_cff")

process.load("DQMServices.Core.DQM_cfg")
process.DQM.collectorHost = ''

process.load("Geometry.CMSCommonData.cmsIdealGeometryXML_cfi")

process.load("CondCore.DBCommon.CondDBCommon_cfi")

process.load("CalibTracker.SiStripLorentzAngle.SiStripCalibLorentzAngle_cfi")

process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))

process.source = cms.Source("EmptySource",
    numberEventsInRun = cms.untracked.uint32(1),
    firstRun = cms.untracked.uint32(1)
)

process.MessageLogger = cms.Service("MessageLogger",
    debug = cms.untracked.PSet(
        threshold = cms.untracked.string('DEBUG')
    ),
    destinations = cms.untracked.vstring('debug')
)

process.TrackerGeometricDetESModule = cms.ESProducer("TrackerGeometricDetESModule",
    fromDDD = cms.bool(True)
)

process.Timing = cms.Service("Timing")

process.CondDBCommon.connect = 'sqlite_file:LA_CRAFT.db'

process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    connect = cms.string('sqlite_file:LA_CRAFT.db'),
    timetype = cms.untracked.string('runnumber'),
    BlobStreamerName = cms.untracked.string('TBufferBlobStreamingService'),
    DBParameters = cms.PSet(
        authenticationPath = cms.untracked.string('/afs/cern.ch/cms/DB/conddb')
    ),
    toPut = cms.VPSet(cms.PSet(
	record = cms.string('SiStripLorentzAngleRcd'),
        tag = cms.string('SiStripLA_CRAFT_V2P-V3P_220')
    ))
)

#process.print = cms.OutputModule("AsciiOutputModule")

process.p = cms.Path(process.sistripLACalib)
process.sistripLACalib.ModuleFit2ITXMin = -0.4
process.sistripLACalib.ModuleFit2ITXMax = 0.1
process.sistripLACalib.p0_guess = -0.1
process.sistripLACalib.p1_guess = 0.5
process.sistripLACalib.p2_guess = 1
process.sistripLACalib.FitCuts_Entries = 1000
process.sistripLACalib.FitCuts_p0 = 10
process.sistripLACalib.FitCuts_p1 = 0.3
process.sistripLACalib.FitCuts_p2 = 1
process.sistripLACalib.FitCuts_chi2 = 10
process.sistripLACalib.FitCuts_ParErr_p0 = 0.001

process.sistripLACalib.fileName = 'Summary_V2P-V3P_Chiochia.root'

process.sistripLACalib.out_fileName = 'LA_CRAFT_Chiochia_NoP0Cut.root'
process.sistripLACalib.LA_Report = 'LA_Report_Chiochia_NoP0Cut.txt'
process.sistripLACalib.LA_ProbFit = 'LA_ProbFit_Chiochia_NoP0Cut.txt'
process.sistripLACalib.treeName = 'ModuleTree_Chiochia_NoP0Cut.root'


