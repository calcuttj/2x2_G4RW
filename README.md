# 2x2_G4RW

Repository to start developing utils for using Geant4Reweight for 2x2 studies.

# Setting up
This code can be ran easily on a dunegpvm. The required packages are 
<pre>
  dune_pardata
  geant4reweight
  edepsim
  boost
</pre>
which can be set up by sourcing the provided `setup.sh` script.

The code can be built after sourcing the setup script and running `make`.

# Configuration
The `reweight.fcl` file contains the configuration used to run the weighting code in `do_weighting.c++`.

# Running weighting/processing code
The processing can be run on a file produced by edep-sim.
<pre>
  ./do_weighting --c reweight.fcl --i [input].root --o [outputname].root
</pre>

The following is an example produced by `draw_len.py` using two files: one produced with the nominal pi+ cross section
(which was then reweighted by a factor of 2) and one produced by biasing (in edep-sim) the cross section by a factor of 2.

#![Alt text](https://github.com/calcuttj/2x2_G4RW/blob/develop/plots/weight_example.pdf)
