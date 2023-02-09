# Photon Analysis
This is a minimum working example for neutral pion analysis.
To run a macro, login to `virgo.hpc.gsi.de` and download from git:

`git clone https://github.com/AlProzo/photon_analysis.git `

Then, write in terminal:

 - `make`
 - `. run.sh`

## [STEP 1](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L120)
Using ParticleCand loop find all tracks that has a good match with EMC(ECAL) and save it in `emcMultiplicity[sectors][cells]`

## [STEP 2](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L147)
Using `HEmcCluster` select photon candidates with following cuts

- no match to the RPC detector - charged particle veto
- no match to any track in HADES
- beta cut around 1 within 3 sigma resolution ( photons are massles)
- minimum EMC energy (to reduce noisy background)

## [STEP 3](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L182)
Combine any photon pairs into a particle and calculate an invariant mass of this particle, put them into histogram (uncomment _[line 85](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L85) (histogram declaration)_)

and apply cut on minimum opening angle between two photons - _it is needed due to the clustering procedure since close tracks are counted as one cluster_

## [STEP 4](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L204)
Make a mixed-event pairs using `heventmixer.h` class and apply the same cuts as for same-event pairs.  + Uncomment _[line 88-91](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L88) (event mixer declaration)_

## [STEP 5](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L232)
Make a background normalization


#
The output will be in output_test.root, run:

`root -l -n output_test.root`

`hMgg->Draw()`

`hMggMix->Draw("same")`
