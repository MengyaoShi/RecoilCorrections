#include "HTT-utilities/RecoilCorrections/interface/RecoilCorrector.h"

RecoilCorrector::RecoilCorrector(TString fileName) {

  TString cmsswBase = TString( getenv ("CMSSW_BASE") );
  TString baseDir = cmsswBase + "/src";

  _fileName = baseDir+"/"+fileName;
  TFile * file = new TFile(_fileName);
  if (file->IsZombie()) {
    std::cout << "file " << _fileName << " is not found...   quitting " << std::endl;
    exit(-1);
  }

  TH1D * projH = (TH1D*)file->Get("projH");
  TString perpZStr = projH->GetXaxis()->GetBinLabel(1);
  TString paralZStr = projH->GetXaxis()->GetBinLabel(2);

  TH1D * ZPtBinsH = (TH1D*)file->Get("ZPtBinsH");
  int nZPtBins = ZPtBinsH->GetNbinsX();
  float ZPtBins[10];
  TString ZPtStr[10];
  for (int i=0; i<=nZPtBins; ++i) {
    ZPtBins[i] = ZPtBinsH->GetXaxis()->GetBinLowEdge(i+1);
    if (i<nZPtBins)
      ZPtStr[i] = ZPtBinsH->GetXaxis()->GetBinLabel(i+1);
  }

  TH1D * nJetBinsH = (TH1D*)file->Get("nJetBinsH");
  int nJetsBins = nJetBinsH->GetNbinsX();
  TString nJetsStr[5];
  for (int i=0; i<nJetsBins; ++i) {
    nJetsStr[i] = nJetBinsH->GetXaxis()->GetBinLabel(i+1);
  }

  InitMEtWeights(file,
		 perpZStr,
		 paralZStr,
		 nZPtBins,
		 ZPtBins,
		 ZPtStr,
		 nJetsBins,
		 nJetsStr);

  _epsrel = 1e-4;
  _epsabs = 1e-4;

}

RecoilCorrector::~RecoilCorrector() {

}

void RecoilCorrector::InitMEtWeights(TFile * _file,
				     TString  _perpZStr,
				     TString  _paralZStr,
				     int nZPtBins,
				     float * ZPtBins,
				     TString * _ZPtStr,
				     int nJetsBins,
				     TString * _nJetsStr) {

	std::vector<float> newZPtBins;
	std::vector<std::string> newZPtStr;
	std::vector<std::string> newNJetsStr;

	std::string newPerpZStr  = std::string(_perpZStr);
	std::string newParalZStr = std::string(_paralZStr);

	for (int idx=0; idx<nZPtBins+1; ++idx) 
	  newZPtBins.push_back(ZPtBins[idx]);
	for (int idx=0; idx<nZPtBins; ++idx ) 
	  newZPtStr.push_back(std::string(_ZPtStr[idx]));

	for (int idx=0; idx<nJetsBins; ++idx)
	  newNJetsStr.push_back(std::string(_nJetsStr[idx]));

	InitMEtWeights(_file,
		       newZPtBins,
		       newPerpZStr,
		       newParalZStr,
		       newZPtStr,
		       newNJetsStr);
}


void RecoilCorrector::InitMEtWeights(TFile * _fileMet,
				     const std::vector<float>& ZPtBins,
				     const std::string _perpZStr,
				     const std::string _paralZStr,
				     const std::vector<std::string>& _ZPtStr,
				     const std::vector<std::string>& _nJetsStr)
{

  // checking files
  //  if (_fileMet->IsZombie()) {
  //    std::cout << "File " << _fileName << " is not found in directory " << _baseDir << std::endl;
  //    std::cout << "quitting program..." << std::endl;
  //    exit(-1);
  //  }

  _nZPtBins = ZPtBins.size()-1; // the -1 is on purpose!
  _nJetsBins = _nJetsStr.size();
  
  _ZPtBins = ZPtBins;

  for (int ZPtBin=0; ZPtBin<_nZPtBins; ++ZPtBin) {
    for (int jetBin=0; jetBin<_nJetsBins; ++jetBin) {

      TString binStrPerpData  = _perpZStr  + "_" + _nJetsStr[jetBin] + _ZPtStr[ZPtBin] + "_data";
      TString binStrParalData = _paralZStr + "_" + _nJetsStr[jetBin] + _ZPtStr[ZPtBin] + "_data";
      TString binStrPerpMC    = _perpZStr  + "_" + _nJetsStr[jetBin] + _ZPtStr[ZPtBin] + "_mc";
      TString binStrParalMC   = _paralZStr + "_" + _nJetsStr[jetBin] + _ZPtStr[ZPtBin] + "_mc";

      _metZParalData[ZPtBin][jetBin] = (TF1*)_fileMet->Get(binStrParalData);
      _metZPerpData[ZPtBin][jetBin]  = (TF1*)_fileMet->Get(binStrPerpData);
      _metZParalMC[ZPtBin][jetBin]   = (TF1*)_fileMet->Get(binStrParalMC);
      _metZPerpMC[ZPtBin][jetBin]    = (TF1*)_fileMet->Get(binStrPerpMC);


      // checking functions
      if (_metZParalData[ZPtBin][jetBin]==NULL) {
	std::cout << "Function with name " << binStrParalData
		  << " is not found in file " << _fileName << "... quitting program..." << std::endl;
	exit(-1);

      }
      if (_metZPerpData[ZPtBin][jetBin]==NULL) {
	std::cout << "Function with name " << binStrPerpData
		  << " is not found in file " << _fileName << "... quitting program..." << std::endl;
	exit(-1);
	
      }

      if (_metZParalMC[ZPtBin][jetBin]==NULL) {
	std::cout << "Function with name " << binStrParalMC
		  << " is not found in file " << _fileName << "... quitting program..." << std::endl;
	exit(-1);

      }
      if (_metZPerpMC[ZPtBin][jetBin]==NULL) {
	std::cout << "Function with name " << binStrPerpMC
		  << " is not found in file " << _fileName << "... quitting program..." << std::endl;
	exit(-1);
	
      }

      // Met Paral Data

      std::cout << _ZPtStr[ZPtBin] << " : " << _nJetsStr[jetBin] << std::endl;
      
      double xminD,xmaxD;

      _metZParalData[ZPtBin][jetBin]->GetRange(xminD,xmaxD);
      _xminMetZParalData[ZPtBin][jetBin] = float(xminD);
      _xmaxMetZParalData[ZPtBin][jetBin] = float(xmaxD);

      _metZPerpData[ZPtBin][jetBin]->GetRange(xminD,xmaxD);
      _xminMetZPerpData[ZPtBin][jetBin] = float(xminD);
      _xmaxMetZPerpData[ZPtBin][jetBin] = float(xmaxD);

      _metZParalMC[ZPtBin][jetBin]->GetRange(xminD,xmaxD);
      _xminMetZParalMC[ZPtBin][jetBin] = float(xminD);
      _xmaxMetZParalMC[ZPtBin][jetBin] = float(xmaxD);

      _metZPerpMC[ZPtBin][jetBin]->GetRange(xminD,xmaxD);
      _xminMetZPerpMC[ZPtBin][jetBin] = float(xminD);
      _xmaxMetZPerpMC[ZPtBin][jetBin] = float(xmaxD);

      _xminMetZParal[ZPtBin][jetBin] = TMath::Max(_xminMetZParalData[ZPtBin][jetBin],_xminMetZParalMC[ZPtBin][jetBin]);
      _xmaxMetZParal[ZPtBin][jetBin] = TMath::Min(_xmaxMetZParalData[ZPtBin][jetBin],_xmaxMetZParalMC[ZPtBin][jetBin]);

      _xminMetZPerp[ZPtBin][jetBin] = TMath::Max(_xminMetZPerpData[ZPtBin][jetBin],_xminMetZPerpMC[ZPtBin][jetBin]);
      _xmaxMetZPerp[ZPtBin][jetBin] = TMath::Min(_xmaxMetZPerpData[ZPtBin][jetBin],_xmaxMetZPerpMC[ZPtBin][jetBin]);
      
    }
  }

}

void RecoilCorrector::Correct(float MetPx,
			      float MetPy,
			      float genVPx, 
			      float genVPy,
			      float visVPx,
			      float visVPy,
			      int njets,
			      float & MetCorrPx,
			      float & MetCorrPy) {
  
  // input parameters
  // MetPx, MetPy - missing transverse momentum 
  // genVPx, genVPy - generated transverse momentum of Z(W)
  // visVPx, visVPy - visible (measured) transverse momentum of Z(W)
  // njets - number of jets 
  // MetCorrPx, MetCorrPy - corrected missing transverse momentum

  float Zpt = TMath::Sqrt(genVPx*genVPx + genVPy*genVPy);

  float U1 = 0;
  float U2 = 0;
  float metU1 = 0;
  float metU2 = 0;

  CalculateU1U2FromMet(MetPx,
		       MetPy,
		       genVPx,
		       genVPy,
		       visVPx,
		       visVPy,
		       U1,
		       U2,
		       metU1,
		       metU2);
  if (Zpt>1000)
    Zpt = 999;

  if (njets>=_nJetsBins)
    njets = _nJetsBins - 1;

  int ZptBin = binNumber(Zpt, _ZPtBins);

  
  TF1 * metZParalData = _metZParalData[ZptBin][njets];
  TF1 * metZPerpData  = _metZPerpData[ZptBin][njets];
  
  TF1 * metZParalMC   = _metZParalMC[ZptBin][njets];
  TF1 * metZPerpMC     = _metZPerpMC[ZptBin][njets];
  
  if (U1>_xminMetZParal[ZptBin][njets]&&U1<_xmaxMetZParal[ZptBin][njets]) {
    
    int nSumProb = 1;
    double q[1];
    double sumProb[1];
    
    sumProb[0] = metZParalMC->IntegralOneDim(_xminMetZParalMC[ZptBin][njets],U1,_epsrel,_epsabs,_error);
    
    if (sumProb[0]<0) {
      //	std::cout << "Warning ! ProbSum[0] = " << sumProb[0] << std::endl;
      sumProb[0] = 1e-10;
    }
    if (sumProb[0]>1) {
      //	std::cout << "Warning ! ProbSum[0] = " << sumProb[0] << std::endl;
      sumProb[0] = 0.9999999;
    }
    
      
    metZParalData->GetQuantiles(nSumProb,q,sumProb);

    float U1reco = float(q[0]);
    
    if (U1reco>_xminMetZParal[ZptBin][njets]&&U1reco<_xmaxMetZParal[ZptBin][njets]) 
      U1 = U1reco;
    
  }

  if (U2>_xminMetZPerp[ZptBin][njets]&&U2<_xmaxMetZPerp[ZptBin][njets]) {
    
    int nSumProb = 1;
    double q[1];
    double sumProb[1];
    
    sumProb[0] = metZPerpMC->IntegralOneDim(_xminMetZPerpMC[ZptBin][njets],U2,_epsrel,_epsabs,_error);
    
    if (sumProb[0]<0) {
      //	std::cout << "Warning ! ProbSum[0] = " << sumProb[0] << std::endl;
      sumProb[0] = 1e-10;
    }
    if (sumProb[0]>1) {
      //	std::cout << "Warning ! ProbSum[0] = " << sumProb[0] << std::endl;
      sumProb[0] = 0.9999999;
    }
    
    metZPerpData->GetQuantiles(nSumProb,q,sumProb);

    float U2reco = float(q[0]);
    
    if (U2reco>_xminMetZPerp[ZptBin][njets]&&U2reco<_xmaxMetZPerp[ZptBin][njets]) 
      U2 = U2reco;
      
  }
  
  CalculateMetFromU1U2(U1,U2,genVPx,genVPy,visVPx,visVPy,MetCorrPx,MetCorrPy);

}

float RecoilCorrector::CorrectionsBySampling(float x, TF1 * funcMC, TF1 * funcData) {

  int nSumProb = 1;
  double q[1];
  double sumProb[1];
  
  double xD = double(x);

  double xminD = 0;
  double xmaxD = 0;

  funcMC->GetRange(xminD,xmaxD);

  float xmin = float(xminD);

  sumProb[0] = funcMC->IntegralOneDim(xmin,xD,_epsrel,_epsabs,_error);
  
  funcData->GetQuantiles(nSumProb,q,sumProb);

  float output = float(q[0]);

  return output;

}

void RecoilCorrector::U1U2CorrectionsByWidth(float & U1, 
					 float & U2,
					 int ZptBin,
					 int njets) {

  if (njets>=_nJetsBins)
    njets = _nJetsBins - 1;
    
  // ********* U1 *************

  float width = U1 - _meanMetZParalMC[ZptBin][njets];

  if (width<0)
    width *= _rmsLeftMetZParalData[ZptBin][njets]/_rmsLeftMetZParalMC[ZptBin][njets];
  else
    width *= _rmsRightMetZParalData[ZptBin][njets]/_rmsRightMetZParalMC[ZptBin][njets];

  U1 = _meanMetZParalData[ZptBin][njets] + width;

  // ********* U2 *************

  width = U2 - _meanMetZPerpMC[ZptBin][njets];


  if (width<0)
    width *= _rmsLeftMetZPerpData[ZptBin][njets]/_rmsLeftMetZPerpMC[ZptBin][njets];
  else 
    width *= _rmsRightMetZPerpData[ZptBin][njets]/_rmsRightMetZPerpMC[ZptBin][njets];

  U2 = _meanMetZPerpData[ZptBin][njets] + width;

}

void RecoilCorrector::CalculateU1U2FromMet(float metPx,
					   float metPy,
					   float genZPx,
					   float genZPy,
					   float diLepPx,
					   float diLepPy,
					   float & U1,
					   float & U2,
					   float & metU1,
					   float & metU2) {
  
  float hadRecX = metPx + diLepPx - genZPx;
  float hadRecY = metPy + diLepPy - genZPy;
  
  float hadRecPt = TMath::Sqrt(hadRecX*hadRecX+hadRecY*hadRecY);
  
  float phiHadRec = TMath::ATan2(hadRecY,hadRecX);
  
  float phiDiMuon = TMath::ATan2(diLepPy,diLepPx);
  
  float phiMEt = TMath::ATan2(metPy,metPx);
  
  float metPt = TMath::Sqrt(metPx*metPx+metPy*metPy);
  
  float phiZ = TMath::ATan2(genZPy,genZPx);
  
  float deltaPhiZHadRec = phiHadRec - phiZ;
  float deltaPhiDiMuMEt = phiMEt - phiDiMuon;
  
  U1 = hadRecPt * TMath::Cos(deltaPhiZHadRec);
  U2 = hadRecPt * TMath::Sin(deltaPhiZHadRec);
  
  metU1 = metPt * TMath::Cos(deltaPhiDiMuMEt);      
  metU2 = metPt * TMath::Sin(deltaPhiDiMuMEt);
}

void RecoilCorrector::CalculateMetFromU1U2(float U1,
					   float U2,
					   float genZPx,
					   float genZPy,
					   float diLepPx,
					   float diLepPy,
					   float & metPx,
					   float & metPy) {
  
  float hadRecPt = TMath::Sqrt(U1*U1+U2*U2);

  float deltaPhiZHadRec = TMath::ATan2(U2,U1);

  float phiZ = TMath::ATan2(genZPy,genZPx);

  float phiHadRec = phiZ + deltaPhiZHadRec;
  
  float hadRecX = hadRecPt*TMath::Cos(phiHadRec);
  float hadRecY = hadRecPt*TMath::Sin(phiHadRec);
  
  metPx = hadRecX + genZPx - diLepPx;
  metPy = hadRecY + genZPy - diLepPy;
}
