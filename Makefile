do_weighting: do_weighting.c++
	g++ -o do_weighting do_weighting.c++ -I ${GEANT4REWEIGHT_INC} \
	-I ${CETLIB_INC} -I ${CETLIB_EXCEPT_INC} -I ${FHICLCPP_INC} \
	-L ${GEANT4REWEIGHT_LIB} -lReweightBaseLib -L ${FHICLCPP_LIB} -lfhiclcpp \
	-L ${CETLIB_EXCEPT_LIB} -lcetlib_except -L ${CETLIB_LIB} -lcetlib \
	-I ${EDEPSIM_INC} -L ${EDEPSIM_LIB} -ledepsim_io -I ${ROOT_INC} \
	-L ${BOOST_LIB} -lboost_program_options \
	-L ${ROOTSYS}/lib \
	-lCore -lImt -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lROOTVecOps -lTree \
	-lTreePlayer -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread \
	-lMultiProc -lROOTDataFrame -pthread -lm -ldl -rdynamic \
	--std=c++17
clean:
	rm do_weighting
