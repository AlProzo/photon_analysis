#include "hades.h"
#include "hloop.h"
#include "htime.h"
#include "hcategory.h"
#include "hcategorymanager.h"
#include "hparticlecand.h"
#include "hparticletracksorter.h"
#include "hparticlebooker.h"
#include "hparticletool.h"
#include "hparticledef.h"
#include "hparticleevtinfo.h"
#include "henergylosscorrpar.h"
#include "hphysicsconstants.h"

#include "hemccluster.h"
#include "hemcdetector.h"
#include "emcdef.h"

#include "TString.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TFile.h"
#include "TH1F.h"
#include "heventmixer.h"

#include "TLorentzVector.h"

#include <iostream>
#include <vector>
using namespace std;

Int_t loopDST(
	TString infileList="/lustre/hades/dst/feb22/gen2test/059/01/root/be2205923194001.hld_dst_feb22.root",
	TString outfile="test.root",Int_t nEvents=50000)
{
	//  infileList : comma seprated file list "file1.root,file2.root" or "something*.root"
	//  outfile    : optional (not used here) , used to store hists in root file
	//  nEvents    : number of events to processed. if  nEvents < entries or < 0 the chain will be processed

	Bool_t isSimulation = kFALSE;   // sim or real data
	//-------------------------------------------------
	// create loop obejct and hades
	HLoop loop(kTRUE);
	//-------------------------------------------------

	//-------------------------------------------------
	// reading input files and decalring containers
	Bool_t ret =kFALSE;
	if(infileList.Contains(",")) {
		ret = loop.addMultFiles(infileList); // file1,file2,file3
	} else{
		ret = loop.addFiles(infileList);  // myroot*.root
	}

	if(ret == 0) {
		cout<<"READBACK: ERROR : cannot find inputfiles : "<<infileList.Data()<<endl;
		return 1;
	}

	if(!loop.setInput("")) {   // all input categories
		cout<<"READBACK: ERROR : cannot read input !"<<endl;
		exit(1);
	} // read all categories
	loop.printCategories();
	loop.printChain();
	//-------------------------------------------------


	//-------------------------------------------------
	// input data
	HCategory* candCat        = (HCategory*)  HCategoryManager::getCategory(catParticleCand);
	HCategory * emcClusterCat = (HCategory *) HCategoryManager::getCategory(catEmcCluster);
	//-------------------------------------------------

	// --------------------------CONFIGURATION---------------------------------------------------
	// At begin of the program (outside the event loop)
	HParticleTrackSorter sorter;
	sorter.setIgnoreInnerMDC(); // do not reject Double_t inner MDC hits
	sorter.init();            // get catgegory pointers etc...

	// add your histograms here
	TFile* out = new TFile(outfile.Data(),"RECREATE");
	out->cd();

	// TH1D* hMgg    = new     TH1D("hMgg",   ";M_#gamma#gamma[MeV/c^{2}];Counts",400,0,800);


  // TH1D* hMggMix = new     TH1D("hMggMix",";M_#gamma#gamma[MeV/c^{2}];Counts",400,0,800);
	// HGenericEventMixerObj < TLorentzVector > eventMixer;
	// eventMixer.setPIDs(1, 1, 7);
	// eventMixer.setBuffSize(30);

	//-------------------------------------------------
	// event loop starts here
	Int_t entries = loop.getEntries();
	if(nEvents < entries && nEvents >= 0 ) entries = nEvents;
	TString filename;

	for (Int_t i = 0; i < entries; i++) {
		Int_t nbytes =  loop.nextEvent(i);         // get next event. categories will be cleared before
		if(nbytes <= 0) { cout<<nbytes<<endl; break; } // last event reached
		if(i%1000== 0) cout<<"event "<<i<<endl;

		if(loop.isNewFile(filename)) { // if the actual filename is needed
			if(!isSimulation) filename = HTime::stripFileName(filename,kTRUE,kFALSE); // get hld file name
		}

		Float_t VertexZ = gHades->getCurrentEvent()->getHeader()->getVertexReco().getZ();
		if(VertexZ<-200 || VertexZ>0) continue;

		HEventHeader  *event_header   = NULL;
		if(!(event_header = loop.getEventHeader())) continue;
		UInt_t TBit   = (UInt_t) event_header->getTBit();
//		if (!(((TBit&8192)==8192)||((TBit&4096)==4096))) continue;




		// ======================================================================= //
		// ==================================STEP 1=============================== //
		// ======================================================================= //


		// loop over particle candidates in event


		Int_t emcMultiplicity[6][163] = { 0 };

		// for (Int_t j = 0; j < candCat->getEntries(); j++) { // loop over   ParticleCand to see if there is a track match to EMC
		// 	HParticleCand * cand = HCategoryManager::getObject(cand, candCat, j);
		// 	if (!cand->isFlagBit(kIsUsed)) continue;
		// 	Int_t emc_index = cand->getEmcInd();
		// 	Float_t emc_mq  = cand->getMetaMatchQualityEmc();
		//
		// 	if (emc_index >= 0 && emc_mq < 2.5) { // if there is a match within 2.5 sigma then the signal is a charged particle
		// 		HEmcCluster * cls    = (HEmcCluster *) emcClusterCat->getObject(emc_index);
		// 		Int_t sector         = cls->getSector();
		// 		Int_t cell           = HEmcDetector::getPositionFromCell(cls->getCell()); //cell in format 0-162
		// 		emcMultiplicity[sector][cell]++;
		// 	}
		// } // end cand loop




		// ======================================================================= //
		// ==================================STEP 2=============================== //
		// ======================================================================= //



		// vector < TLorentzVector > photons;
		//
		// for (Int_t e = 0; e < emcClusterCat->getEntries(); e++) { // loop over all clusters in emc for finding photons
		// 	HEmcCluster * cls = (HEmcCluster *) emcClusterCat->getObject(e);
		// 	Int_t sector               = cls->getSector();
		// 	Int_t cell                 = HEmcDetector::getPositionFromCell(cls->getCell());//cell in format 0-162
		//
		// 	Int_t matchToRpc          = cls->getNMatchedCells();
		// 	Int_t matchToParticleCand = emcMultiplicity[sector][cell];
		//
		// 	Float_t time         = cls->getTime();  //in ns
		// 	Float_t energy       = cls->getEnergy();//in MeV
		// 	HGeomVector track(cls->getXLab(), cls->getYLab(), cls->getZLab()-VertexZ); //
		// 	Double_t trackLength = track.length()/1000; //in meters
		// 	Double_t beta        = trackLength  / (TMath::C() *time * 1.e-9); // l / ct
		//
		// 	if (matchToRpc > 0 ||matchToParticleCand > 0 || beta < 0.8 || beta > 1.2 ||energy<100) continue;  //photon conditions
		//
		// 	track /= track.length(); //normalize direction vector
		// 	track *=  energy; //multiply it by energy to get the momentum vector of a photon
		// 	TLorentzVector photon;
		// 	photon.SetXYZM(track.X(), track.Y(), track.Z(), 0);
		// 	photons.push_back(photon);
		// } // loop over emc





		// ======================================================================= //
		// ==================================STEP 3=============================== //
		// ======================================================================= //


		// if (photons.size()==0) continue;
		//
		// for (size_t g1 = 0; g1 < (photons.size() - 1); g1++) { // first gamma
		// 	for (size_t g2 = g1 + 1; g2 < photons.size(); g2++) { // second gamma
		// 		TLorentzVector photon1 = photons.at(g1);
		// 		TLorentzVector photon2 = photons.at(g2);
		// 		Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg(); //in degrees
		// 		if(openingAngle<6) continue;
		// 		TLorentzVector pion = photon1+photon2; //construct a pion out of 2 gammas
		// 		hMgg->Fill(pion.M());
		// 	}
		// }





		// ======================================================================= //
		// ==================================STEP 4=============================== //
		// ======================================================================= //


		// eventMixer.nextEvent();
		// eventMixer.addVector(photons, 1); //add mixing vector into the buffer
		// vector < pair < TLorentzVector, TLorentzVector >>& pairsVec = eventMixer.getMixedVector(); //get mixed-event pairs from the whole buffer
		//
		// for (Int_t iPair = 0; iPair < pairsVec.size(); iPair++) {
		// 	pair < TLorentzVector, TLorentzVector >& pair = pairsVec[iPair];
		// 	TLorentzVector photon1 = pair.first;
		// 	TLorentzVector photon2 = pair.second;
		// 	Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg();
		// 	if(openingAngle<6) continue;
		// 	TLorentzVector pion = photon1+photon2;
		// 	hMggMix->Fill(pion.M());
		// }





	} // end eventloop




	// ======================================================================= //
	// ==================================STEP 5=============================== //
	// ======================================================================= //


	// Double_t integralMixEvent  = hMggMix->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230)); //region outside pi0 peak
	// Double_t integralSameEvent =    hMgg->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230)); //e.g. 210 - 230 MeV
	// hMggMix->Scale(integralSameEvent/integralMixEvent);


	// out->cd();
	// hMgg->Write();

	// hMggMix->Write();

	out->cd();
	out->Save();
	out->Close();

	delete gHades;
	return 0;
}
