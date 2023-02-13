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
	TString infileList = "/lustre/hades/dst/feb22/gen2test2/059/01/root/be2205923194001.hld_dst_feb22.root",
	TString outfile    = "output_test.root", Int_t nEvents = 50000)
{
	//  infileList : comma seprated file list "file1.root,file2.root" or "something*.root"
	//  outfile    : optional (not used here) , used to store hists in root file
	//  nEvents    : number of events to processed. if  nEvents < entries or < 0 the chain will be processed

	Bool_t isSimulation = kFALSE; // sim or real data

	// -------------------------------------------------
	// create loop obejct and hades
	HLoop loop(kTRUE);
	// -------------------------------------------------

	// -------------------------------------------------
	// reading input files and decalring containers
	Bool_t ret = kFALSE;
	if (infileList.Contains(",")) {
		ret = loop.addMultFiles(infileList); // file1,file2,file3
	} else {
		ret = loop.addFiles(infileList); // myroot*.root
	}

	if (ret == 0) {
		cout << "READBACK: ERROR : cannot find inputfiles : " << infileList.Data() << endl;
		return 1;
	}

	if (!loop.setInput("")) { // all input categories
		cout << "READBACK: ERROR : cannot read input !" << endl;
		exit(1);
	} // read all categories
	loop.printCategories();
	loop.printChain();
	// -------------------------------------------------

	// -------------------------------------------------
	// input data
	HCategory * candCat       = (HCategory *)  HCategoryManager::getCategory(catParticleCand);
	HCategory * emcClusterCat = (HCategory *)  HCategoryManager::getCategory(catEmcCluster);

	const Int_t emcPositionFromCell[255]={
		-1,  -1,  -1,  -1,  -1,  -1,   0,   1,   2,   3,   4,  -1,  -1,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  -1,  -1,   5,   6,   7,   8,   9,  -1,  -1,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  -1,  10,  11,  12,  13,  14,  15,  16,  -1,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  -1,  17,  18,  19,  20,  21,  22,  23,  -1,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  24,  25,  26,  27,  28,  29,  30,  31,  32,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  33,  34,  35,  36,  37,  38,  39,  40,  41,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  -1,  42,  43,  44,  45,  46,  47,  48,  49,  50,  -1,  -1,  -1,  -1,
		-1,  -1,  -1,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  -1,  -1,  -1,
		-1,  -1,  -1,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  -1,  -1,  -1,
		-1,  -1,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  -1,  -1,
		-1,  -1,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  -1,  -1,
		-1,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,  -1,
		-1, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,  -1,
		129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
		146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162
	};
	// -------------------------------------------------

	// --------------------------CONFIGURATION---------------------------------------------------
	// At begin of the program (outside the event loop)
	HParticleTrackSorter sorter;
	sorter.setIgnoreInnerMDC(); // do not reject Double_t inner MDC hits
	sorter.init();            // get catgegory pointers etc...

	// add your histograms here
	TFile * out = new TFile(outfile.Data(), "RECREATE");
	out->cd();
	// histograms for check
	TH1D * hVertexZ                 = new     TH1D("hVertexZ", "Vertex Z; Vertex Z [mm];Counts", 400, -500, 100);
	TH1D * hEmcMq                   = new     TH1D("hEmcMq", "Match quality Emc; Emc Mq;Counts", 400, 0, 10);
	TH1D * hEmcClusterCell          = new     TH1D("hEmcClusterCell", "Cell hits from Emc; sector*200+cell;Counts", 1200, 0, 1200);
	TH1D * hEmcClusterBeta          = new     TH1D("hEmcClusterBeta", "Beta from Emc; #beta;Counts", 500, 0, 1.5);
	TH1D * hEmcClusterTime          = new     TH1D("hEmcClusterTime", "Time from Emc; time [ns];Counts", 1000, -200, 1200);
	TH1D * hEmcClusterTrackLength   = new     TH1D("hEmcClusterTrackLength",
	                                               "Track Length from Emc;Length [mm];Counts",400, 2400, 2800);
	TH1D * hEmcClusterEnergy        = new     TH1D("hEmcClusterEnergy", "Energy from Emc;Energy [MeV];Counts", 500, 0,1000);
	TH1D * hDiPhotonOpeningAngle    = new     TH1D("hDiPhotonOpeningAngle",
	                                               "Opening Angle from 2 photons;#alpha_{#gamma#gamma} [deg];Counts", 90,0, 90);
	TH1D * hDiPhotonOpeningAngleMix = new     TH1D("hDiPhotonOpeningAngleMix",
	                                               "Opening Angle from 2 photons in mixed-event;#alpha_{#gamma#gamma} [deg];Counts", 90, 0, 90);

	// TH1D * hMgg    = new     TH1D("hMgg", "   ;M_{#gamma#gamma}[MeV/c^{2}];Counts", 200, 0, 800);  //  same event invariant mass
	// TH1D * hMggMix = new     TH1D("hMggMix", ";M_{#gamma#gamma}[MeV/c^{2}];Counts", 200, 0, 800);  // mixed event invariant mass


	// HGenericEventMixerObj < TLorentzVector > eventMixer;
	// eventMixer.setPIDs(1, 1, 7); // decay of neutral pion (geant id = 7) to two photons (geant id = 1)
	// eventMixer.setBuffSize(30); // maximum number of events that are used for mixing

	// -------------------------------------------------
	// event loop starts here
	Int_t entries = loop.getEntries();
	if (nEvents < entries && nEvents >= 0) entries = nEvents;
	TString filename;

	for (Int_t i = 0; i < entries; i++) {
		Int_t nbytes = loop.nextEvent(i); // get next event. categories will be cleared before
		if (nbytes <= 0) { cout << nbytes << endl; break; } // last event reached
		if (i % 1000 == 0) cout << "event " << i << endl;

		if (loop.isNewFile(filename)) {                                             // if the actual filename is needed
			if (!isSimulation) filename = HTime::stripFileName(filename, kTRUE, kFALSE); // get hld file name
		}

		Float_t VertexZ = gHades->getCurrentEvent()->getHeader()->getVertexReco().getZ();
		Float_t VertexX = gHades->getCurrentEvent()->getHeader()->getVertexReco().getX();
		Float_t VertexY = gHades->getCurrentEvent()->getHeader()->getVertexReco().getY();

		hVertexZ->Fill(VertexZ);
		if (VertexZ < -200 || VertexZ > 0) continue;

		HEventHeader * event_header = NULL;
		if (!(event_header = loop.getEventHeader())) continue;
		UInt_t TBit = (UInt_t) event_header->getTBit();

		sorter.cleanUp();
		sorter.resetFlags(kTRUE, kTRUE, kTRUE, kTRUE); // reset all flags for flags (0-28) ,reject,used,lepton
		sorter.fill(HParticleTrackSorter::selectHadrons); // fill only good hadrons (already marked good leptons will be skipped)
		sorter.selectBest(Particle::kIsBestRKSorter, Particle::kIsHadronSorter);


		// ======================================================================= //
		// ==================================STEP 1=============================== //
		// ======================================================================= //
		// Using ParticleCand loop find all tracks that has a good match with EMC(ECAL)
		// and add information about it to the HEmcCluster


		for (Int_t j = 0; j < candCat->getEntries(); j++) {
			// 	// loop over  ParticleCand to see if there is a track match to EMC
			// 	HParticleCand * cand = HCategoryManager::getObject(cand, candCat, j);
			// 	if (!cand->isFlagBit(kIsUsed)) continue;
			// 	Int_t emc_index = cand->getEmcInd();
			// 	Float_t emc_mq  = cand->getMetaMatchQualityEmc();
			//
			// 	hEmcMq->Fill(emc_mq);         // check histograms on emc_mq before making a cut
			// 	if (emc_index >= 0 && emc_mq < 3) { // if there is a match within 3 sigma then the signal is a charged particle
			// 		HEmcCluster * cls = (HEmcCluster *) emcClusterCat->getObject(emc_index);
			// 		cls->addMatchedTrack(); // a track has been matched to the particular EMC cluster
			// 	}
		}  // end ParticleCand loop


		// ======================================================================= //
		// ==================================STEP 2=============================== //
		// ======================================================================= //
		// valid for  MAR19 (and further)
		// Using `HEmcCluster` select photon candidates with following cuts
		// - no match to the RPC detector - charged particle veto
		// - no match to any track in HADES
		// - beta cut around 1 within 3 sigma resolution ( photons are massles)
		// - minimum EMC energy (to reduce noisy background)



		vector < TLorentzVector > photons; // vector of all photons in one event
		for (Int_t e = 0; e < emcClusterCat->getEntries(); e++) {  // loop over all clusters in emc for finding photons
			// 	HEmcCluster * cls = (HEmcCluster *) emcClusterCat->getObject(e);
			//
			// 	Bool_t matchToRpc          = (Bool_t) cls->getNMatchedCells();
			// 	Bool_t matchToParticleCand = (Bool_t) cls->getNMatchedTracks();
			// 	// if there is a match to either RPC or track than this is not a neutral particle
			// 	if (matchToRpc || matchToParticleCand) continue;
			//
			// 	Int_t sector      = cls->getSector();
			// 	Int_t cell        = emcPositionFromCell[(Int_t)cls->getCell()];// cell in format 0-162
			// 	// the photon cluster cannot originate in first 2 rows of ECAL(closest to the beam),
			// 	// there is no acceptance for photons due to RICH/MDC material
			// 	hEmcClusterCell->Fill(sector*200+cell);
			// 	if(cell<10) continue;
			//
			// 	Float_t energy = cls->getEnergy();// in MeV
			// 	Float_t time   = cls->getTime();// in ns
			// 	HGeomVector track(cls->getXLab() - VertexX, cls->getYLab() - VertexY, cls->getZLab() - VertexZ);
			// 	// recalculate track from its position from target
			// 	Double_t trackLength = track.length() / 1000; // in meters
			// 	Double_t beta        = trackLength / (TMath::C() * time * 1.e-9);
			// 	// beta = l / ct      --- beta calculation from tracklength and time of flight
			//
			// 	//	Check histograms before making cuts
			// 	hEmcClusterTrackLength->Fill(track.length());
			// 	hEmcClusterTime->Fill(time);
			//
			// 	hEmcClusterBeta->Fill(beta);
			// 	hEmcClusterEnergy->Fill(energy);
			//
			// 	Double_t betaCut = 0.2;                                // symmetric cut value around beta=1
			// 	if (beta < (1 - betaCut) || beta > (1 + betaCut)) continue; // beta cut to remove slow particles (neutrons, mishits)
			//
			// 	Double_t minEnergyCut = 100.;   // in MeV
			// 	if (energy < minEnergyCut) continue; // energy cut to remove noisy background
			//
			// 	track /= track.length(); // normalize direction vector -> (n_x, n_y, n_z)
			// 	track *= energy;   // multiply it by energy to get the momentum vector of a photon : (Px,Py,Pz) =(Ex,Ey,Ez)
			// 	TLorentzVector photon;
			// 	photon.SetXYZM(track.X(), track.Y(), track.Z(), 0);
			// 	photons.push_back(photon);
		}  // loop over emc




		// ======================================================================= //
		// ==================================STEP 3=============================== //
		// ======================================================================= //
		// * Combine any photon pairs into a particle and calculate an invariant mass of this particle,
		// * Put them into histogram (uncomment histogram hMgg declaration and writing)
		// * and apply cut on minimum opening angle between two photons -
		//   it is needed due to the clustering procedure since close tracks are counted as one cluster

		if (photons.size() == 0) continue;
		// // e.g. photons vector = {1,2,3} - all possible combinations are {1,2}, {1,3} and {2,3}
		// for (size_t g1 = 0; g1 < (photons.size() - 1); g1++) { // first gamma
		// 	for (size_t g2 = g1 + 1; g2 < photons.size(); g2++) { // second gamma
		// 		TLorentzVector photon1 = photons.at(g1);
		// 		TLorentzVector photon2 = photons.at(g2);
		//
		// 		Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg();
		// 		hDiPhotonOpeningAngle->Fill(openingAngle);
		// 		Float_t minOpeningAngle = 6;
		// 		if (openingAngle < minOpeningAngle) continue;
		//
		// 		TLorentzVector pion = photon1 + photon2; // construct a pion out of 2 gammas
		// 		hMgg->Fill(pion.M());
		// 	}
		// }


		// ======================================================================= //
		// ==================================STEP 4=============================== //
		// ======================================================================= //
		//	Make a mixed-event pairs using heventmixer.h class
		//  and apply the same cuts as for same-event pairs.
		//	+uncomment event mixer declaration and hMggMix histogram writing


		// // declare a new event for event mixer
		// eventMixer.nextEvent();
		// // add a current event photon vector into the buffer
		// eventMixer.addVector(photons, 1);
		// // get mixed-event pairs from the whole buffer,using current and previous events
		// vector < pair < TLorentzVector, TLorentzVector >>& pairsVec = eventMixer.getMixedVector();
		//
		// for (Size_t iPair = 0; iPair < pairsVec.size(); iPair++) {
		// 	pair < TLorentzVector, TLorentzVector >& pair = pairsVec[iPair]; // unique pair of 2 photons from different events
		// 	TLorentzVector photon1 = pair.first;                       // first photon from event X
		// 	TLorentzVector photon2 = pair.second;// second photon from event Y
		//
		// 	Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg();
		// 	hDiPhotonOpeningAngleMix->Fill(openingAngle);
		// 	Float_t minOpeningAngle = 6;
		// 	if (openingAngle < minOpeningAngle) continue; // same condition as in STEP 3
		//
		// 	TLorentzVector pion = photon1 + photon2;
		// 	hMggMix->Fill(pion.M());
		// }




	} // end eventloop


	// ======================================================================= //
	// ==================================STEP 5=============================== //
	// ======================================================================= //
	//	Make a background normalization


	// Double_t integralMixEvent  = hMggMix->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230)); //region outside pi0 peak
	// Double_t integralSameEvent =    hMgg->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230)); //e.g. 210 - 230 MeV
	// hMggMix->Scale(integralSameEvent/integralMixEvent);// perform the background normalization

	// ======================================================================= //

	//	Writing

	out->cd();

	hVertexZ->Write();
	hEmcMq->Write();
	hEmcClusterCell->Write();
	hEmcClusterTrackLength->Write();
	hEmcClusterBeta->Write();
	hEmcClusterTime->Write();
	hEmcClusterEnergy->Write();
	hDiPhotonOpeningAngle->Write();
	hDiPhotonOpeningAngleMix->Write();

	hMgg->Write();
	hMggMix->Write();

	out->cd();
	out->Save();
	out->Close();

	delete gHades;
	return 0;
} /* loopDST */
