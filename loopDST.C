#include "TObject.h"
#include "TROOT.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"

#include "TString.h"
#include "TMath.h"
#include "TLorentzVector.h"

#include "TList.h"
#include "TTree.h"
#include "TH1.h"

#include "hloop.h"
#include "hcategorymanager.h"
#include "htool.h"
#include "htime.h"
#include "hphysicsconstants.h"
#include "hparticletracksorter.h"

#include "heventheader.h"
#include "hparticleevtinfo.h"
#include "hgeantheader.h"
#include "hparticlecand.h"
#include "hparticlecandsim.h"
#include "hgeantkine.h"

#include "emcdef.h"
#include "hemccluster.h"

#include "heventmixer.h" // Custom Event Mixer class

using namespace std;



Int_t loopDST(
	TString infileList = "/lustre/hades/dst/feb22/gen2test2/059/01/root/be2205923194001.hld_dst_feb22.root",
	TString outfile    = "output_test.root", Int_t nEventsDesired = 50000)
{
	//--------------------------------------------------------------------------------
	// Initialization of the global ROOT object and the Hades Loop
	// The Hades Loop used as an interface to the DST data (Basically a container of a TChain).
	// kTRUE - The global HADES object is being created if not existing
	//--------------------------------------------------------------------------------
	TROOT trDSTAnalysis("trDSTAnalysis", "Simple DST analysis Macro");
	new HLoop(kTRUE); // Later accessed via the global gLoop variable!

	//--------------------------------------------------------------------------------
	// The following block finds / adds the input DST files to the HLoop
	//--------------------------------------------------------------------------------
	Bool_t ret = kFALSE;
	if (infileList.Contains(",")) {
		ret = gLoop->addMultFiles(infileList);     // file1,file2,file3
	} else {
		ret = gLoop->addFiles(infileList);     // myroot*.root
	}

	if (ret == 0) {
		cout << "READBACK: ERROR : cannot find inputfiles : " << infileList.Data() << endl;
		return 1;
	}


	//--------------------------------------------------------------------------------
	// Booking the categories to be read from the DST files.
	// By default all categories are booked therefore -* (Unbook all) first and book the ones needed
	// All required categories have to be booked except the global Event Header which is always booked
	//--------------------------------------------------------------------------------
	if (!gLoop->setInput("*")) {
		cerr << "ERROR: Failed to book categories!" << endl;
		return 1;
	}

	//--------------------------------------------------------------------------------
	// Setting the cache size of the HLoop internal TChain to read data from the DSTs
	// Improves performance of the lustre storage by decreasing load on lustre META servers
	//--------------------------------------------------------------------------------
	gLoop->getChain()->SetCacheSize(8ll*1024ll*1024ll); // 8 MB
	gLoop->getChain()->AddBranchToCache("*", kTRUE);
	gLoop->getChain()->StopCacheLearningPhase();

	gLoop->printCategories(); // Just for informative purposes

	//--------------------------------------------------------------------------------
	// Get category pointers (They have to be booked!)
	//--------------------------------------------------------------------------------

	HCategory * categoryParticleCand       = (HCategory *)  HCategoryManager::getCategory(catParticleCand);
	HCategory * categoryEmcCluster         = (HCategory *)  HCategoryManager::getCategory(catEmcCluster);

	if (!categoryEmcCluster || !categoryParticleCand) { // If a required category does not exist the macro makes no sense
		cerr << "ERROR: Required category is missing!" << endl;
		return 1;
	}


	//================================================================================================================================================================
	// Put your object declarations here
	//================================================================================================================================================================

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


	TH1D * hVertexZ                 = new     TH1D("hVertexZ", "Vertex Z; Vertex Z [mm];Counts", 400, -500, 100);
	TH1D * hEmcMq                   = new     TH1D("hEmcMq", "Match quality Emc; Emc Mq;Counts", 400, 0, 10);
	TH1D * hEmcClusterSize          = new     TH1D("hEmcClusterSize", "Cluster size of hits; cluster size ;Counts", 10, 0, 10);
	TH1D * hEmcClusterCell          = new     TH1D("hEmcClusterCell", "Cell hits from Emc; sector*200+cell;Counts", 1200, 0, 1200);
	TH1D * hEmcClusterBeta          = new     TH1D("hEmcClusterBeta", "Beta from Emc; #beta;Counts", 500, 0, 1.5);
	TH1D * hEmcClusterTime          = new     TH1D("hEmcClusterTime", "Time from Emc; time [ns];Counts", 1000, -200, 1200);
	TH1D * hEmcClusterTrackLength   = new     TH1D("hEmcClusterTrackLength",
	                                               "Track Length from Emc;Length [mm];Counts",400, 2400, 2800);
	TH1D * hEmcClusterEnergy        = new     TH1D("hEmcClusterEnergy", "Energy from Emc;Energy [MeV];Counts", 500, 0,1000);
	TH1D * hPhotonMultiplicity      = new     TH1D("hPhotonMultiplicity", "Number of photons per event; photon number ;Events", 20, 0, 20);
	TH1D * hDiPhotonOpeningAngle    = new     TH1D("hDiPhotonOpeningAngle",
	                                               "Opening Angle from 2 photons;#alpha_{#gamma#gamma} [deg];Counts", 90,0, 90);
	TH1D * hDiPhotonOpeningAngleMix = new     TH1D("hDiPhotonOpeningAngleMix",
	                                               "Opening Angle from 2 photons in mixed-event;#alpha_{#gamma#gamma} [deg];Counts", 90, 0, 90);

	// TH1D * hMgg    = new     TH1D("hMgg", " Invariant mass of 2 same-event photons;M_{#gamma#gamma}[MeV/c^{2}];Counts", 200, 0, 800);   //  same event invariant mass
	// TH1D * hMggMix = new     TH1D("hMggMix", "Invariant mass of 2 mixed-event photons;M_{#gamma#gamma}[MeV/c^{2}];Counts", 200, 0, 800);   // mixed event invariant mass




	//--------------------------------------------------------------------------------
	// The following counter histogram is used to gather some basic information on the analysis
	//--------------------------------------------------------------------------------
	enum eCounters {
		cNumAllEvents  = 1,
	};

	TH1D* hCounter = new TH1D("hCounter", "", 1, 0., 1.);
	hCounter->GetXaxis()->SetBinLabel(1, "All Events");
	hCounter->Fill(-1., 0.);     // For whatever reason this is required to merge the histogram correctly!


	//--------------------------------------------------------------------------------
	// Creating and initializing the Event Mixer
	//--------------------------------------------------------------------------------
	// HGenericEventMixerObj < TLorentzVector > eventMixer;
	// eventMixer.setPIDs(1, 1, 7);  // decay of neutral pion (geant id = 7) to two photons (geant id = 1)
	// eventMixer.setBuffSize(30);  // maximum number of events that are used for mixing


	// -------------------------------------------------
	//--------------------------------------------------------------------------------
	// Creating and initializing the track sorter and a simple stopwatch object
	//--------------------------------------------------------------------------------
	HParticleTrackSorter sorter;
	sorter.init();

	//--------------------------------------------------------------------------------
	// The amount of events to be processed
	//--------------------------------------------------------------------------------
	Long64_t nEvents = gLoop->getEntries();
	if (nEventsDesired < nEvents && nEventsDesired >= 0)
		nEvents = nEventsDesired;


	//--------------------------------------------------------------------------------
	// The global event loop which loops over all events in the DST files added to HLoop
	// The loop breaks if the end is reached
	//--------------------------------------------------------------------------------
	for (Long64_t event = 0; event < nEvents; ++event) {
		if (gLoop->nextEvent(event) <= 0) {
			cout << " Last event processed " << endl;
			break;
		}

		//--------------------------------------------------------------------------------
		// Just the progress of the analysis
		//--------------------------------------------------------------------------------
		HTool::printProgress(event, nEvents, 1, "Analyzed events: ");

		//--------------------------------------------------------------------------------
		// The Event Header Object containing general information
		//-------------------------------------------------------------------------------
		HEventHeader* eventHeader = gLoop->getEventHeader();

		//--------------------------------------------------------------------------------
		// Reject calibration, beginrun, endrun, ... events - Only for real data Events - Comment out for Simulataions!
		//--------------------------------------------------------------------------------


		hCounter->AddBinContent(cNumAllEvents);
		UInt_t TBit = (UInt_t) eventHeader->getTBit();

		Float_t VertexZ = eventHeader->getVertexReco().getZ();
		Float_t VertexX = eventHeader->getVertexReco().getX();
		Float_t VertexY = eventHeader->getVertexReco().getY();

		hVertexZ->Fill(VertexZ);
		if (VertexZ < -200 || VertexZ > 0) continue;

		sorter.cleanUp();
		sorter.resetFlags(kTRUE, kTRUE, kTRUE, kTRUE); // reset all flags for flags (0-28) ,reject,used,lepton
		sorter.fill(HParticleTrackSorter::selectHadrons); // fill only good hadrons (already marked good leptons will be skipped)
		//sorter.selectBest(HParticleTrackSorter::kIsBestRK, HParticleTrackSorter::kIsHadron); // Comment this line in for any Hydra version older than 2.5.4
		sorter.selectBest(Particle::kIsBestRKSorter, Particle::kIsHadronSorter);             // Comment this line in for any Hydra version newer or equal 2.5.4

		// ======================================================================= //
		// ==================================STEP 1=============================== //
		// ======================================================================= //
		// Using ParticleCand loop find all tracks that has a good match with EMC(ECAL)
		// and add information about it to the HEmcCluster


		// for (Int_t iTrack = 0; iTrack < categoryParticleCand->getEntries(); iTrack++) {
		// 	// loop over  ParticleCand to see if there is a track match to EMC
		// 	HParticleCand * cand = (HParticleCand*) categoryParticleCand->getObject(iTrack);
		// 	if (!cand->isFlagBit(kIsUsed)) continue;
		// 	Int_t emcIndex = cand->getEmcInd();
		// 	Float_t emcMatchQuality  = cand->getMetaMatchQualityEmc();

		// 	hEmcMq->Fill(emcMatchQuality);         // check histograms on emcMatchQuality before making a cut
		// 	if (emcIndex >= 0 && emcMatchQuality < 3) { // if there is a match within 3 sigma then the signal is a charged particle
		// 		HEmcCluster * cls = (HEmcCluster *) categoryEmcCluster->getObject(emcIndex);
		// 		cls->addMatchedTrack(); // a track has been matched to the particular EMC cluster
		// 	}
		// }  // end ParticleCand loop


		// ======================================================================= //
		// ==================================STEP 2=============================== //
		// ======================================================================= //
		// valid for  MAR19 (and further)
		// Using `HEmcCluster` select photon candidates with following cuts
		// - no match to the RPC detector - charged particle veto
		// - no match to any track in HADES
		// - beta cut around 1 within 3 sigmaz resolution ( photons are massles)
		// - minimum EMC energy (to reduce noisy background)



		vector < TLorentzVector > photons; // vector of all photons in one event
		for (Int_t iEmcCluster = 0; iEmcCluster < categoryEmcCluster->getEntries(); iEmcCluster++) {  // loop over all clusters in emc for finding photons
			// HEmcCluster * cls = (HEmcCluster *) categoryEmcCluster->getObject(iEmcCluster);

			// Bool_t matchToRpc          = (Bool_t) cls->getNMatchedCells();
			// Bool_t matchToParticleCand = (Bool_t) cls->getNMatchedTracks();
			// // if there is a match to either RPC or track than this is not a neutral particle
			// if (matchToRpc || matchToParticleCand) continue;

			// Int_t sector      = cls->getSector();
			// Int_t cell        = emcPositionFromCell[(Int_t)cls->getCell()];// cell in format 0-162
			// // the photon cluster cannot originate in first 2 rows of ECAL(closest to the beam),
			// // there is no acceptance for photons due to RICH/MDC material
			// hEmcClusterCell->Fill(sector*200+cell);
			// if(cell<10) continue;

			// Float_t energy = cls->getEnergy();// in MeV
			// Float_t time   = cls->getTime();// in ns
			// HGeomVector track(cls->getXLab() - VertexX, cls->getYLab() - VertexY, cls->getZLab() - VertexZ);
			// // recalculate track from its position from target
			// Double_t trackLength = track.length() / 1000; // in meters
			// Double_t beta        = trackLength / (TMath::C() * time * 1.e-9);
			// // beta = l / ct      --- beta calculation from tracklength and time of flight

			// //	Check histograms before making cuts
			// hEmcClusterTrackLength->Fill(track.length());
			// hEmcClusterTime->Fill(time);

			// hEmcClusterBeta->Fill(beta);
			// hEmcClusterEnergy->Fill(energy);

			// Double_t betaCut = 0.2;                                // symmetric cut value around beta=1
			// if (beta < (1 - betaCut) || beta > (1 + betaCut)) continue; // beta cut to remove slow particles (neutrons, mishits)

			// Double_t minEnergyCut = 100.;   // in MeV
			// if (energy < minEnergyCut) continue; // energy cut to remove noisy background

			// track /= track.length(); // normalize direction vector -> (n_x, n_y, n_z)
			// track *= energy;   // multiply it by energy to get the momentum vector of a photon : (Px,Py,Pz) =(Ex,Ey,Ez)
			// TLorentzVector photon;
			// photon.SetXYZM(track.X(), track.Y(), track.Z(), 0);
			// photons.push_back(photon);

			// hEmcClusterSize->Fill((Size_t)cls->getNCells());
		}  // loop over emc




		// ======================================================================= //
		// ==================================STEP 3=============================== //
		// ======================================================================= //
		// * Combine any photon pairs into a particle and calculate an invariant mass of this particle,
		// * Put them into histogram (uncomment histogram hMgg - declaration (line 116) and writing (line 324))
		// * and apply cut on minimum opening angle between two photons -
		//   it is needed due to the clustering procedure since close tracks are counted as one cluster
		hPhotonMultiplicity->Fill(photons.size());

		if (photons.size() == 0) continue;


		// //e.g. photons vector = {1,2,3} - all possible combinations are {1,2}, {1,3} and {2,3}
		// for (size_t g1 = 0; g1 < (photons.size() - 1); g1++) { // first gamma
		// 	for (size_t g2 = g1 + 1; g2 < photons.size(); g2++) { // second gamma
		// 		TLorentzVector photon1 = photons.at(g1);
		// 		TLorentzVector photon2 = photons.at(g2);

		// 		Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg();
		// 		hDiPhotonOpeningAngle->Fill(openingAngle);
		// 		Float_t minOpeningAngle = 6;
		// 		if (openingAngle < minOpeningAngle) continue;

		// 		TLorentzVector pion = photon1 + photon2; // construct a pion out of 2 gammas
		// 		hMgg->Fill(pion.M());
		// 	}
		// }


		// ======================================================================= //
		// ==================================STEP 4=============================== //
		// ======================================================================= //
		//	Make a mixed-event pairs using heventmixer.h class
		//  and apply the same cuts as for same-event pairs.
		//	+uncomment event mixer declaration (lines 120-122) and hMggMix histogram declaration (line 117) and writing (line 325)


		// // declare a new event for event mixer
		// eventMixer.nextEvent();
		// // add a current event photon vector into the buffer
		// eventMixer.addVector(photons, 1);
		// // get mixed-event pairs from the whole buffer,using current and previous events
		// vector < pair < TLorentzVector, TLorentzVector >>& pairsVec = eventMixer.getMixedVector();

		// for (Size_t iPair = 0; iPair < pairsVec.size(); iPair++) {
		// 	pair < TLorentzVector, TLorentzVector >& pair = pairsVec[iPair]; // unique pair of 2 photons from different events
		// 	TLorentzVector photon1 = pair.first;                       // first photon from event X
		// 	TLorentzVector photon2 = pair.second;// second photon from event Y

		// 	Float_t openingAngle = photon1.Angle(photon2.Vect()) * TMath::RadToDeg();
		// 	hDiPhotonOpeningAngleMix->Fill(openingAngle);
		// 	Float_t minOpeningAngle = 6;
		// 	if (openingAngle < minOpeningAngle) continue; // same condition as in STEP 3

		// 	TLorentzVector pion = photon1 + photon2;
		// 	hMggMix->Fill(pion.M());
		// }


	} // End of event loop

//--------------------------------------------------------------------------------
// Doing some cleanup and finalization work
//--------------------------------------------------------------------------------
	sorter.finalize();
	cout << "Finished DST processing" << endl;


// ======================================================================= //
// ==================================STEP 5=============================== //
// ======================================================================= //
//	Make a background normalization


	// Double_t integralMixEvent  = hMggMix->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230));//region outside pi0 peak
	// Double_t integralSameEvent =    hMgg->Integral(hMggMix->FindBin(210),hMggMix->FindBin(230));//e.g. 210 - 230 MeV
	// hMggMix->Scale(integralSameEvent/integralMixEvent);// perform the background normalization

// ======================================================================= //



//--------------------------------------------------------------------------------
// Creating output file and storing results there
//--------------------------------------------------------------------------------
	TFile* out = new TFile(outfile.Data(), "RECREATE");
	out->cd();


//================================================================================================================================================================
// Remember to write your results to the output file here
//================================================================================================================================================================
	hCounter->Write();
	hVertexZ->Write();
	hEmcMq->Write();
	hEmcClusterCell->Write();
	hEmcClusterTrackLength->Write();
	hEmcClusterBeta->Write();
	hEmcClusterTime->Write();
	hEmcClusterEnergy->Write();
	hEmcClusterSize->Write();
	hPhotonMultiplicity->Write();
	hDiPhotonOpeningAngle->Write();
	hDiPhotonOpeningAngleMix->Write();

	//hMgg->Write();
	//hMggMix->Write();


//--------------------------------------------------------------------------------
// Closing file and finalization
//--------------------------------------------------------------------------------
	out->Save();
	out->Close();

	cout << "####################################################" << endl;
	return 0;


} /* loopDST */
