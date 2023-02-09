# photon_analysis
This is a minimum working example for neutral pion analysis involving the following:

## [STEP 1](https://github.com/AlProzo/photon_analysis/blob/main/loopDST.C#L120)
Using ParticleCand loop find all tracks that has a good match with EMC(ECAL)

## STEP 2
Using `HEmcCluster` select photon candidates with following cuts

- no match to the RPC detector
- no match to any track in HADES
- beta cut around 1 
- minimum EMC energy(to reduce noisy background)

## STEP 3
Combine any photon pairs into a particle and calculate an invariant mass of this particle, 
( * and apply cut on minimum opening angle between two photons 6 degrees  - it is needed due to the clustering procedure : 
close tracks are counted as one cluster *)

## STEP 4
Make a mixed-event pairs using `heventmixer.h` class and apply the same cuts as for same-event pairs

## STEP 5
Make a background normalization
