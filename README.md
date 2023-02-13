# Photon Analysis
This is a minimum working example for neutral pion analysis.
To run a macro, login to `virgo-debian10.hpc.gsi.de` and download the project from git:

`git clone https://github.com/AlProzo/photon_analysis.git `

Then, write in terminal:
 - ` . /cvmfs/hadessoft.gsi.de/install/debian10/6.24.02/hydra2-6.3/defall.sh` (latest HYDRA version)
 - `make`
 - `. run.sh`

## [STEP 1](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L157)
		 Using ParticleCand loop find all tracks that has a good match with EMC(ECAL)
		 and add information about it to the `HEmcCluster`


## [STEP 2](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L179)
Using `HEmcCluster` select photon candidates with following cuts

- no match to the RPC detector - charged particle veto
- no match to any track in HADES
- beta cut around 1 within 3 sigma resolution ( photons are massles)
- minimum EMC energy (to reduce noisy background)

## [STEP 3](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#238)
* Combine any photon pairs into a particle and calculate an invariant mass of this particle, 

* Put them into histogram (uncomment histogram declaration on _[line 116](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L116)_ and writing on _[line 324](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L324)_)

* and apply cut on minimum opening angle between two photons - _it is needed due to the clustering procedure since close tracks are counted as one cluster_

## [STEP 4](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L264)
Make a mixed-event pairs using `heventmixer.h` class and apply the same cuts as for same-event pairs.  

+Uncomment _[line 117-122](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L117) (event mixer declaration)_ , 
_[line 325](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L325) (histogram writing)_

## [STEP 5](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L299)
Make a background normalization


#
The output will be in output_test.root, run:

`root -l -n output_test.root`

`hMgg->Draw()`

`hMggMix->Draw("same")`
