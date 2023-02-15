# Photon Analysis
This is a minimum working example for neutral pion analysis.
To run a macro, login to `virgo-debian10.hpc.gsi.de` and download the project from git:

```bash
git clone https://github.com/AlProzo/photon_analysis.git 
```

Then, write in terminal:
 - ` . /cvmfs/hadessoft.gsi.de/install/debian10/6.24.02/hydra2-6.2/defall.sh` (HYDRA version)
 - `make`
 - `./analysis   /lustre/hades/dst/feb22/gen2test2/059/05/root/be2205919453905.hld_dst_feb22.root output_test.root 30000`
    `./(executable) (input file)  (output file)  (number of events)`

## [STEP 1](loopDST.C#L221)
Using ParticleCand loop find all tracks that has a good match with EMC(ECAL)
and add information about it to the `HEmcCluster`


## [STEP 2](loopDST.C#L243)
Using `HEmcCluster` select photon candidates with following cuts

- no match to the RPC detector - charged particle veto
- no match to any track in HADES
- beta cut around 1 ( photons are massles)
- minimum EMC energy (to reduce noisy background)

## [STEP 3](loopDST.C#L304)
* Combine any photon pairs into a particle and calculate an invariant mass of this particle, 

* Put them into histogram (uncomment histogram declaration _[line 138](loopDST.C#L138)_ and writing _[line 408](loopDST.C#L408)_)

* and apply cut on minimum opening angle between two photons - _it is needed due to the clustering procedure since close tracks are counted as one cluster_

## [STEP 4](loopDST.C#L333)
Make a mixed-event pairs using `heventmixer.h` class and apply the same cuts as for same-event pairs.  

+Uncomment _[line 159-161](loopDST.C#L159) (event mixer declaration)_ , _[line 139](loopDST.C#L139) (histogram declaration)_
_[line 409](loopDST.C#L409) (histogram writing)_

## [STEP 5](loopDST.C#L372)
Make a background normalization


#
The output will be in output_test.root, run:

`root -l -n output_test.root`

`hMgg->Draw()`

`hMggMix->Draw("same")`
