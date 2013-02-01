#!/bin/bash

# PARSE OPTIONS ################################################################

#usage: ./combo_new.sh -g 30 -t 1000 -m 'r' -k 'd' -d 112 -a 13

# Default parameters
offset=0
decimation=0
GAIN=20 
TIME=0
ARFCN=7
mode="v"
direction="d"
configuration=""
frequency=900000000
GOHOME=~/people_counter/airprobe/gsm-receiver/src/python/
DATAPATH=~/people_counter/data
UHDRECORDERPATH=uhd_rx_cfile.py
USRPRECORDERPATH=my_usrp_rx_cfile.py
MODULESLOT=A
driver=uhd

# Parse options
while getopts '?m:f:o:d:g:s:t:a:k:c:' OPTION
do
	case $OPTION in
	m)	mode="$OPTARG"
		;;
	f)	frequency="$OPTARG"
		;;
	o) 	offset="$OPTARG"
		;;
	d)    	decimation="$OPTARG"
		;;
	g)    	GAIN="$OPTARG"
		;;
	s)   	MODULESLOT="$OPTARG"
		;;
	t)   	TIME="$OPTARG"
		;;
	a)   	ARFCN="$OPTARG"
		;;
	k)   	direction="$OPTARG"
		;;
	c)   	configuration="$OPTARG"
		;;
	?)	echo "This is a tool for USRP beginners who just want to record"
		echo "some samples to file or view them with baudline"
		echo ""
		echo "Usage $0 [-m mode] [-f frequency] [-o offset] [-d decimation]"
		echo ""
		echo " -m .... mode can be 'view' for viewing live data with baudline (default)"
		echo "         or 'record' for recording into a file and 'replay' for playback."
		echo ""
		echo " -f .... frequency can be a frequency number in Hz or one of the"
		echo "         following station names:"
		echo "         'skyper' ....... German POCSAG pager service"
		echo "         'intertechno' .. Wireless socket brand"
		echo "         'db0bar' ....... HAM-Relais here in Berlin/BB"
		echo "         'thermo' ....... Thermo sensor (unkown brand)"
		echo "         'lisa' ......... Humantechnik Lisa (dorbell light signal system)"
		echo ""
		echo " -o .... offset allows you to add a frequency offset to the frequency"
		echo "         you have specified"
		echo ""
		echo " -d .... decimation must be even and can be from 4 to 256"
		echo ""
		echo " -g .... gain"
		echo ""
		exit 2
		;;
	esac
done

displayCommand="cat "

if [ $driver == usrp ]; then
	captureCommand="$USRPRECORDERPATH "
	captureCommand+="-R $MODULESLOT "
else
	captureCommand="$UHDRECORDERPATH "
	captureCommand+="--wire-format=sc16 -g $GAIN --scalar=1 "
fi

	

################################################################################################
#################################        targetfile      #######################################
################################################################################################

if [ $mode == "v" ]; then
	echo " * Setting up interprocess communication:"

	if [ -p ./fifo.pipe ]
	then
		echo "    * Pipe already initalized."
	else
		echo "    * Pipe does not exist, creating a new one..."
		mkfifo fifo.pipe
	fi

	targetfile="./fifo.pipe"

elif [ $mode == "r" ] || [ $mode == "rv" ]; then
	targetfile="samples.bin"
	echo " * Targetfile is: $targetfile"
else
	echo " * Error: Invalid mode specified!"
	exit 2
fi

displayCommand+="$targetfile | baudline -reset -format le32f "

################################################################################################
#####################        decimation/samplerate      #######################################
################################################################################################

#setting default DECIMATION value unless specified
if [ $mode == "v" ]; then
	if [ $decimation == 0 ]; then
		decimation=10
	fi
fi
if [ $mode == "r" ] || [ $mode == "rv" ]; then
	if [ $decimation == 0 ]; then
		decimation=112
	fi
fi

# Calculate SAMPLERATE from decimation
let samplerate=64000000/$decimation

if [ $driver == usrp ]; then
	captureCommand+="-d $decimation "	
else
	captureCommand+="--samp-rate $samplerate "
fi

displayCommand+="-samplerate $samplerate -channels 2 -quadrature "
if [ $driver == uhd ]; then
	displayCommand+="-scaleby 10000 "
fi
displayCommand+="-stdin &"

################################################################################################
#################################        number of samples     #################################
################################################################################################
if [ $TIME == 0 ]; then
	echo " Time not set."
else
	let samples=$samplerate*$TIME/1000
	captureCommand+="-N $samples "
fi

################################################################################################
#################################       frequency     ##########################################
################################################################################################

frequency=`arfcncalc -a $ARFCN -$direction | cut -c 1-9`
captureCommand+="-f $frequency "

################################################################################################
#######################       recorded data filename  ##########################################
################################################################################################

datafilename="$DATAPATH/capture_${ARFCN}_${decimation}_${direction}.cfile"

captureCommand+="$targetfile"

if [ $mode == "v" ]; then
	captureCommand+=" &"
fi

# Show configuration
echo " * CONFIGURATION:"
echo " * Decimation scaler is: $decimation"
echo " * Samplerate is set to: $samplerate"
echo " * Frequency set to: $frequency"
echo " * Time set to $TIME ms."	
echo " * Capturing $samples samples."
echo " * Mode is: $mode"
echo " * Gain is: $GAIN"
if [ $direction == 'u' ]; then
	echo " * Capturing UpLink"
elif [ $direction == 'd' ]; then
	echo " * Capturing DownLink"
else 
	echo " * Wrong Link Direction Specified"
	exit 1
fi
echo "Use $0 -? to display help information"
echo ""

decodeCommand="./go.sh $datafilename $decimation $configuration"

echo " * Capture command is: $captureCommand" 
echo " * display command is: $displayCommand" 
echo " * decode  command is: $decodeCommand" 

# Launch capture process
echo " * Capturing data, press ENTER to stop...";
echo "------------------------------------8<---------------------------------------"

eval $captureCommand

if [ $driver==uhd ]; then
	echo " * Process uhd_rx_cfile.py started."
else
	echo " * Process usrp_rx_cfile.py started."
fi


if [ $mode == "v" ]; then
	echo " * Displaying live data with baudline..."
	eval $displayCommand
elif [ $mode == "r" ] || [ $mode == "rv" ]; then
	echo " * Recording data..."
fi


# Clean up
if [ $mode == "v" ]; then
	read $trash
	echo " * Terminating baudline..."
	killall baudline
fi

echo " * Terminating usrp..."
killall python



if [ $driver == uhd ] && [ $mode != 'v' ]; then
	./1000multiplier.py
fi

echo " * Recording ended."

if [ $mode == "r" ] || [ $mode == "rv" ]; then
	echo cp samples.bin "$datafilename"
	cp samples.bin "$datafilename"
	if [ $driver == uhd ]; then 
		echo cp samples_mul.bin "$datafilename"
		cp samples_mul.bin "$datafilename"		
	fi
	cd $GOHOME
	echo "Running:"
	echo $decodeCommand
	eval $decodeCommand
	let samplerate=64000000/$decimation
	if [ $mode == "rv" ]; then
		echo "cat $datafilename | baudline -reset -format le32f -samplerate $samplerate -channels 2 -quadrature -stdin &"
		echo "Displaying Record"
		cat $datafilename | baudline -reset -format le32f -samplerate $samplerate -channels 2 -quadrature -stdin &
		read $trash
		echo " * Terminating baudline..."
		killall baudline
	fi
fi




