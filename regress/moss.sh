#!/bin/sh

rm -f moss.pix moss.log moss.png moss2.pix

cat > moss.asc << EOF
I 0 v4
Gary Moss's "World on a Platter"
S 16 tor 6 4.916235923767e+00 -3.280223083496e+01 3.171177673340e+01 0.000000000000e+00 5.079999923706e+00 0.000000000000e+00 -1.796051025391e+01 0.000000000000e+00 1.796051025391e+01 -1.796051025391e+01 0.000000000000e+00 -1.796051025391e+01 -1.436841011047e+01 0.000000000000e+00 1.436841011047e+01 -1.436840915680e+01 0.000000000000e+00 -1.436840915680e+01 -2.155261230469e+01 0.000000000000e+00 2.155261230469e+01 -2.155261230469e+01 0.000000000000e+00 -2.155261230469e+01
S 20 platform.s 1 8.971723937988e+01 -6.957164001465e+01 -2.376846313477e+01 0.000000000000e+00 1.390160369873e+02 0.000000000000e+00 0.000000000000e+00 1.390160369873e+02 6.950803756714e+00 0.000000000000e+00 0.000000000000e+00 6.950803756714e+00 -1.390160369873e+02 0.000000000000e+00 0.000000000000e+00 -1.390160369873e+02 1.390160369873e+02 0.000000000000e+00 -1.390160369873e+02 1.390160369873e+02 6.950803756714e+00 -1.390160369873e+02 0.000000000000e+00 6.950803756714e+00
S 20 box.s 1 3.002833557129e+01 -5.211529731750e+00 -1.637908935547e+01 0.000000000000e+00 2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 2.679275512695e+01 2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 2.679275512695e+01 -2.679275512695e+01 0.000000000000e+00 0.000000000000e+00 -2.679275512695e+01 2.679275512695e+01 0.000000000000e+00 -2.679275512695e+01 2.679275512695e+01 2.679275512695e+01 -2.679275512695e+01 0.000000000000e+00 2.679275512695e+01
S 18 cone.s 5 1.687542724609e+01 -3.474353027344e+01 -1.637908935547e+01 0.000000000000e+00 0.000000000000e+00 2.644671630859e+01 9.350327491760e+00 -9.350327491760e+00 0.000000000000e+00 9.350327491760e+00 9.350327491760e+00 0.000000000000e+00 4.339659690857e+00 -4.339659690857e+00 0.000000000000e+00 1.453863143921e+00 1.453863143921e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00
S 19 ellipse.s 4 1.613092041016e+01 4.665559387207e+01 -3.722520828247e+00 1.487607192993e+01 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 8.980256080627e+00 -8.980256080627e+00 0.000000000000e+00 8.980256080627e+00 8.980256080627e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00
C Y platform.r 1000 0 1 -16081 0 0 1 80 150 230 0 0 0
M + platform.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16268
C Y box.r 2000 0 1 -16558 0 0 1 100 190 190 0 0 0
M + box.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16519
C Y ellipse.r 4000 0 1 16619 0 0 1 100 210 100 0 0 0
M + ellipse.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
C Y cone.r 3000 0 1 -16209 0 0 1 255 100 255 0 0 0
M + cone.s 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
C Y tor.r 6000 0 1 -16219 0 0 1 240 240 0 0 0 0
M + tor 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 16473
S 19 LIGHT 11 2.015756225586e+01 -1.352595329285e+01 5.034742355347e+00 2.539999961853e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.539999961853e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.539999961853e+00 6.652935973394e-32 -1.768683290104e-12 -4.437394240169e-21 0.000000000000e+00 -5.077455353676e-19 -2.153920654297e+03 0.000000000000e+00 0.000000000000e+00 -5.077455353676e-19 -5.400174699785e-19 0.000000000000e+00 0.000000000000e+00
C Y light.r 1000 0 1 0 1 100 1 255 255 255 1 0 0
light
M u LIGHT 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
C N all.g -1 0 6 16619 0 0 0 0 0 0 0 0 0
M u platform.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
M u box.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 -2.369892883301e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 1.340998744965e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 8.023991584778e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
M u cone.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 2.204924011230e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 1.223486709595e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 2.111249841619e-07 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
M u ellipse.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.467929267883e+01 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 -4.160771179199e+01 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 3.879878234863e+01 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
M u tor.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
M u light.r 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 0.000000000000e+00 1.000000000000e+00 0
EOF

../src/conv/asc2g moss.asc moss.g

echo "rendering moss..."
../src/rt/rt -P 1 -B -C0/0/50 -M -s 512 -o moss.pix moss.g all.g > moss.log 2>&1 << EOF
viewsize 1.572026215e+02;
eye_pt 6.379990387e+01 3.271768951e+01 3.366661453e+01;
viewrot -5.735764503e-01 8.191520572e-01 0.000000000e+00
0.000000000e+00 -3.461886346e-01 -2.424038798e-01 9.063078165e-01
0.000000000e+00 7.424039245e-01 5.198368430e-01 4.226182699e-01
0.000000000e+00 0.000000000e+00 0.000000000e+00 0.000000000e+00
1.000000000e+00 ;
start 0;
end;
EOF


if [ ! -f moss.pix ] ; then
	echo raytrace failed
else
	if [ ! -f $1/regress/mosspix.asc ] ; then
		echo No reference file for moss.pix
	else
		../src/conv/asc2pix < $1/regress/mosspix.asc > moss_ref.pix
		../src/util/pixdiff/pixdiff moss.pix \
			moss_ref.pix \
			> moss.pix.diff \
			2> moss-diff.log

		/bin/echo -n moss.pix
		tr , '\012' < moss-diff.log | grep many
	fi
fi

../src/util/pix-png -s 512 moss.pix > moss.png
../src/util/png-pix moss.png > moss2.pix
../src/util/pixdiff moss.pix moss2.pix > moss_png.diff 2> moss-png.log
NUMBER_WRONG=`tr , '\012' < moss-png.log | awk '/many/ {print $1}'`
echo moss.pix $NUMBER_WRONG off by many

if [ $NUMBER_WRONG == 0 ] ; then
    /bin/echo '-> moss.sh succeeded'
else
    /bin/echo '-> moss.sh failed'
endif

exit $NUMBER_WRONG

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
