#include "CalibTracker/SiStripLorentzAngle/interface/LA_Filler_Fitter.h"
#include "CalibTracker/SiStripLorentzAngle/interface/TTREE_FOREACH_ENTRY.hh"
#include "CalibTracker/SiStripLorentzAngle/interface/Book.h"

#include <cmath>
#include <boost/foreach.hpp>
#include <boost/regex.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <TF1.h>
#include <TTree.h>

void LA_Filler_Fitter::
fill(TTree* tree, Book& book) {
  TTREE_FOREACH_ENTRY(tree,
		      std::vector<unsigned>* LEAF( clusterdetid )
		      std::vector<unsigned>* LEAF( clusterwidth )
		      std::vector<float>*    LEAF( clustervariance )
		      std::vector<unsigned>* LEAF( tsostrackmulti )
		      std::vector<int>*      LEAF( tsostrackindex )
		      std::vector<float>*    LEAF( tsosdriftx )
		      std::vector<float>*    LEAF( tsosdriftz )
		      std::vector<float>*    LEAF( tsoslocalpitch )
		      std::vector<float>*    LEAF( tsoslocaltheta )
		      std::vector<float>*    LEAF( tsoslocalphi )
		      std::vector<float>*    LEAF( tsosBdotY )
		      std::vector<double>*   LEAF( trackchi2ndof )
		      std::vector<unsigned>* LEAF( trackhitsvalid )
		      ) {
    if(maxEvents_) TTREE_FOREACH_ENTRY_total = std::min(maxEvents_,TTREE_FOREACH_ENTRY_total);
    for(unsigned i=0; i< clusterwidth->size() ; i++) {  

      SiStripDetId detid((*clusterdetid)[i]);

      if( (*tsostrackmulti)[i] != 1 ||
	  (*trackchi2ndof)[(*tsostrackindex)[i]] > 10 ||
	  (*trackhitsvalid)[(*tsostrackindex)[i]] < 7 ||
	  detid.subDetector()!=SiStripDetId::TIB && 
	  detid.subDetector()!=SiStripDetId::TOB        ) 
	continue;
      
      float BdotY = (*tsosBdotY)[i];
      float driftx = (*tsosdriftx)[i];
      float driftz = (*tsosdriftz)[i];

      int sign = driftx < 0  ?  1  :  -1 ;
      double projection = driftz * tan((*tsoslocaltheta)[i]) * cos((*tsoslocalphi)[i]);
      unsigned width = (*clusterwidth)[i];
      float sqrtVar = sqrt((*clustervariance)[i]);
      float pitch = (*tsoslocalpitch)[i];

      poly<std::string> granular;
      if(ensembleBins_) {
	granular+= "ensembleBin"+boost::lexical_cast<std::string>((int)(ensembleBins_*(sign*driftx/driftz-ensembleLow_)/(ensembleUp_-ensembleLow_)));
	granular+= "_pitch"+boost::lexical_cast<std::string>((int)(pitch*10000));
	granular+= "";
	if(ensembleSize_) granular*= "_sample"+boost::lexical_cast<std::string>(TTREE_FOREACH_ENTRY_index % ensembleSize_);
      } else {
	granular+= detid.subDetector()==SiStripDetId::TOB? "TOB" : "TIB";
	granular+= "";
	if(byLayer_) { unsigned layer = detid.subDetector() == SiStripDetId::TOB ? TOBDetId(detid()).layer() : TIBDetId(detid()).layer(); 
                       granular*= "_layer"+boost::lexical_cast<std::string>(layer)+(detid.stereo()?"s":"a"); }
	if(byModule_)  granular*= "_module"+boost::lexical_cast<std::string>(detid());
      }
      poly<std::string> A1("_all"); 
      if(width==1) A1*="_width1";

      if(ensembleBins_==0)   book.fill( fabs(BdotY),                     granular+"_field"            , 101, 1, 5 );
      float N=2.5;           book.fill( sign*driftx/driftz,              granular+"_reconstruction"   , 101, -N*pitch/driftz, N*pitch/driftz          );
      if(methods_ & RATIO)   book.fill( sign*projection/driftz,          granular+ method(RATIO,0)+A1 , 101, -N*pitch/driftz, N*pitch/driftz          );
      if(methods_ & WIDTH)   book.fill( sign*projection/driftz,   width, granular+ method(WIDTH,0)    , 101, -N*pitch/driftz, N*pitch/driftz, 20,0,20 );
      if(methods_ & SQRTVAR) book.fill( sign*projection/driftz, sqrtVar, granular+ method(SQRTVAR,0)  , 101, -N*pitch/driftz, N*pitch/driftz, 100,0,2 );
    }
  }
}

void LA_Filler_Fitter::
make_and_fit_ratio(Book& book, bool cleanup) {
  for(Book::const_iterator it = book.begin(".*"+method(RATIO,0)+"_width1"); it!=book.end(); ++it) {
    std::string base = boost::erase_all_copy(it.name(),"_width1");

    std::string width1 = base+"_width1";
    std::string all    = base+"_all";
    std::string ratio  = base+"_ratio";

    TH1* p = book.book(ratio, (TH1*) book(width1)->Clone(ratio.c_str()));
    p->Divide(book(all));
    p->Fit("gaus","LLQ");
    double mean = p->GetFunction("gaus")->GetParameter(1);
    double sigma = p->GetFunction("gaus")->GetParameter(2);
    p->Fit("gaus","LLMEQ","",mean-sigma,mean+sigma);

    if(cleanup) {
      book.erase(width1);
      book.erase(all);
    }
  }
}

void LA_Filler_Fitter::
make_and_fit_profile(Book& book, const std::string& key, bool cleanup) {
  for(Book::const_iterator hist2D = book.begin(".*"+key); hist2D!=book.end(); ++hist2D) {
    std::string name = hist2D.name()+"_profile";
    TH1* p = book.book(name, (TH1*) ((TH2*)(*hist2D))->ProfileX(name.c_str()));
    float min = p->GetMinimum();
    float max = p->GetMaximum();
    float xofmin = p->GetBinCenter(p->GetMinimumBin()); if( xofmin>0.0 || xofmin<-0.15) xofmin = -0.05;
    float xofmax = p->GetBinCenter(p->GetMaximumBin());

    TF1* fit = new TF1("LA_profile_fit","[2]*(TMath::Abs(x-[0]))+[1]",-1,1);
    fit->SetParLimits(0,-0.15,0.01);
    fit->SetParLimits(1, 0.6*min, 1.25*min );
    fit->SetParLimits(2,0.1,10);
    fit->SetParameters( xofmin, min, (max-min) / fabs( xofmax - xofmin ) );
    p->Fit(fit,"IMEQ");
    if( p->GetFunction("LA_profile_fit")->GetChisquare() / p->GetFunction("LA_profile_fit")->GetNDF() > 5 ||
	p->GetFunction("LA_profile_fit")->GetParError(0) > 0.03) 
      p->Fit(fit,"IMEQ");

    if(cleanup) book.erase(hist2D.name());
  }
}

LA_Filler_Fitter::Result LA_Filler_Fitter::
result(Method m, const std::string name, const Book& book) {
  Result p;
  std::string base = boost::erase_all_copy(name,method(m));
  if(book.contains(base+"_reconstruction")) {
    p.reco    = book(base+"_reconstruction")->GetMean();
    p.recoErr = book(base+"_reconstruction")->GetRMS();
  }
  if(book.contains(base+"_field"))
    p.field = book(base+"_field")->GetMean();

  if(book.contains(name)) {
    p.entries = (unsigned)(book(name)->GetEntries());
    switch(m) {
    case RATIO: {
      TF1* f = book(name)->GetFunction("gaus");
      p.measure = f->GetParameter(1);
      p.measureErr = f->GetParError(1);
      p.chi2 = f->GetChisquare();
      p.ndof = f->GetNDF();
      break; }
    case WIDTH: case SQRTVAR: {
      TF1* f = book(name)->GetFunction("LA_profile_fit");
      p.measure = f->GetParameter(0);
      p.measureErr = f->GetParError(0);
      p.chi2 = f->GetChisquare();
      p.ndof = f->GetNDF();
      break;
    case NONE: break;
    }
    }
  }
  return p;
}

std::map<uint32_t,LA_Filler_Fitter::Result> LA_Filler_Fitter::
module_results(const Book& book, const Method m) {
  std::map<uint32_t,Result> results;
  for(Book::const_iterator it = book.begin(".*module.*"+method(m)); it!=book.end(); ++it ) {
    uint32_t detid = boost::lexical_cast<uint32_t>( boost::regex_replace( it.name(),
									  boost::regex(".*_module(\\d*)_.*"),
									  std::string("\1")));
    results[detid] = result(m,it.name(),book);
  }
  return results;
}

std::map<std::string,LA_Filler_Fitter::Result> LA_Filler_Fitter::
layer_results(const Book& book, const Method m) {
  std::map<std::string,Result> results;
  for(Book::const_iterator it = book.begin(".*layer.*"+method(m)); it!=book.end(); ++it ) {
    std::string name = boost::erase_all_copy(it.name(),method(m));
    results[name] = result(m,it.name(),book);
  }
  return results;
}

std::map<std::string, std::vector<LA_Filler_Fitter::Result> > LA_Filler_Fitter::
ensemble_results(const Book& book, const Method m) {
  std::map<std::string, std::vector<Result> > results;
  for(Book::const_iterator it = book.begin(".*_sample.*"+method(m)); it!=book.end(); ++it ) {
    std::string name = boost::regex_replace(it.name(),boost::regex("sample\\d*_"),"");
    results[name].push_back(result(m,it.name(),book));
  }
  return results;
}

void LA_Filler_Fitter::
summarize_ensembles(Book& book) {
  typedef std::map<std::string, std::vector<Result> > results_t;
  results_t results;
  if(methods_ & SQRTVAR) { results_t g = ensemble_results(book,SQRTVAR); results.insert(g.begin(),g.end());}
  if(methods_ & RATIO)   { results_t g = ensemble_results(book,RATIO);   results.insert(g.begin(),g.end());}
  if(methods_ & WIDTH)   { results_t g = ensemble_results(book,WIDTH);   results.insert(g.begin(),g.end());}
  
  BOOST_FOREACH(const results_t::value_type group, results) {
    const std::string name = group.first;
    BOOST_FOREACH(const Result p, group.second) {
      if( p.chi2/p.ndof > 5 ) continue;
      float pad = (ensembleUp_-ensembleLow_)/10;
      book.fill( p.reco,       name+"_ensembleReco", 12*ensembleBins_, ensembleLow_-pad, ensembleUp_+pad );
      book.fill( p.measure,    name+"_measure",      12*ensembleBins_, ensembleLow_-pad, ensembleUp_+pad );
      book.fill( p.measureErr, name+"_merr",         500, 0, 0.1);
    }
    book(name+"_measure")->Fit("gaus","LMEQ");
    book(name+"_merr")->Fit("gaus","LMEQ");
  }
}

std::map<std::string, std::vector<LA_Filler_Fitter::EnsembleSummary> > LA_Filler_Fitter::
ensemble_summary(const Book& book) {
  std::cout << "begin ensemble_summary" << std::endl;
  std::map<std::string, std::vector<EnsembleSummary> > summary;
  for(Book::const_iterator it = book.begin(".*_ensembleReco"); it!=book.end(); ++it) {
    std::string base = boost::erase_all_copy(it.name(),"_ensembleReco");

    TH1* reco = *it;
    TH1* measure = book(base+"_measure");
    TH1* merr = book(base+"_merr");

    EnsembleSummary s;
    s.truth = reco->GetMean();
    s.meanMeasured = measure->GetFunction("gaus")->GetParameter(1);
    s.SDmeanMeasured = measure->GetFunction("gaus")->GetParError(1);
    s.sigmaMeasured = measure->GetFunction("gaus")->GetParameter(2);
    s.SDsigmaMeasured = measure->GetFunction("gaus")->GetParError(2);
    s.meanUncertainty = merr->GetFunction("gaus")->GetParameter(1);
    s.SDmeanUncertainty = merr->GetFunction("gaus")->GetParError(1);

    std::string name = boost::regex_replace(base,boost::regex("ensembleBin\\d*_"),"");
    summary[name].push_back(s);
  }
  return summary;
}

std::ostream& operator<<(std::ostream& strm, const LA_Filler_Fitter::Result& r) { 
  return strm << r.reco    <<"\t"<< r.recoErr <<"\t"
	      << r.measure <<"\t"<< r.measureErr <<"\t"
	      << r.field <<"\t"
	      << r.chi2 <<"\t"
	      << r.ndof <<"\t"
	      << r.entries;
}

std::ostream& operator<<(std::ostream& strm, const LA_Filler_Fitter::EnsembleSummary& e) { 
  return strm << e.truth <<"\t"
	      << e.meanMeasured    <<"\t"<< e.SDmeanMeasured <<"\t"
	      << e.sigmaMeasured   <<"\t"<< e.SDsigmaMeasured <<"\t"
	      << e.meanUncertainty <<"\t"<< e.SDmeanUncertainty;
}


