echo "Sourcing shared.sh"

function run_wrapper
{
	OPTS=$1
	CORELIST=$2
	PROG=$3
	LOGFILES=$4
	WORKLOAD=$5

	# Build (one for configuration)
	if [[ $OPTS == *-p* ]]; then
		echo -n "Building sk_$PROG (static) .. "
		(CXXFLAGS=-DSHL_STATIC make "sk_$PROG" > /dev/null) || error "Build failed"
	else
		echo -n "Building sk_$PROG .. "
		(make "sk_$PROG" > /dev/null) || error "Build failed"
	fi
	echo "done"

	# Cores
	# --------------------------------------------------
	for CORES in $CORELIST; do

		# Create and remember log files
		TMP=`mktemp /tmp/tmp-test_all-XXXXXX`
		echo $TMP " " $PROG " " $OPTS " " $CORES >> $LOGFILES
		echo "Running: $TMP $PROG $OPTS $CORES"

		# Run
		scripts/run.sh $OPTS $PROG $CORES ours $WORKLOAD &> $TMP
		RC=$?

		# Evaluate return code
		if [[ $RC -eq 0 ]]; then
			RCS=$txtgrn"ok  "$txtrst
		else
			RCS=$txtred"fail"$txtrst
		fi

		# Print result
		echo -e " .. return code [$RCS] for [$PROG] with [$CORES] [$OPTS] was [$RC] ... log at [$TMP]"
	done

	return $RC

}
