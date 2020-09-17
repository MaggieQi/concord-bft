FOLDER=~/archipelago/paper/rslt
protocol=$2
EXPDIR=$FOLDER/$protocol/$1
number_operations=100
commit_duration=100
beta=1
if [ "$3" != "" ]; then
    beta=$3
    if [ "$beta" != "1" ]; then
        number_operations=$((50*$beta))
    fi
    echo "Set batch size: $beta number of operations: $number_operations"
fi
numclientservers=31

mkdir -p $EXPDIR
mkdir config
if [ "$1" == "client_inc" ]; then
    echo "RUN $protocol client increase experiment"
    numservers=4
    best=''
    best_throughput=0.0
    best_latency=100000.0
    for ((i=2;i<512;i=i*2)); do
        if [ "$protocol" == "archipelago" ]; then
            python generate_config.py --file_prefix=config/test --protocol=$protocol --client_batchsize=$beta --number_operations=$number_operations --commit_duration=$commit_duration $numservers $numservers $numclientservers $i
        else
            python generate_config.py --file_prefix=config/test --protocol=$protocol --replica_batchsize=$beta --number_operations=$number_operations $numservers $numservers $numclientservers $i
        fi
        python run.py config/test_servers_$numservers\_clients_$i\_$protocol.json
        python analyze.py latest_rslt/config.json
        dest=$EXPDIR/client_inc_servers_$numservers\_clients_$i\_beta_$beta\_commit_$commit_duration.json
        cp latest_rslt/rslt.json $dest

        throughput=`grep \"throughput\" $dest | awk '{print $2;}'`
        latency=`grep \"50_totalorder_latency\" $dest | awk '{split($2,s,",");print s[1];}'`

        latency_td=$(echo "$latency < $best_latency*2.0" | bc -l)
        throughput_td=$(echo "$throughput > $best_throughput*1.1" | bc -l)

        echo "throughput: $throughput latency:$latency throughput_td:$throughput_td latency_td:$latency_td"

        if [ "$throughput_td" == "1" ] && [ "$latency_td" == "1" ]; then
            best_throughput=$throughput
            best_latency=$latency
            best=$dest
        else
            break
        fi
    done
    mkdir -p $EXPDIR\_result
    cp $best $EXPDIR\_result/

    echo "best: $best"
    grep \"throughput\" $best
    grep \"50_latency\" $best
    grep \"50_totalorder_latency\" $best
elif [ "$1" == "server_inc" ]; then
    echo "RUN $protocol server increase experiment"
    for i in 1 2 3 4 5 10; do
        numservers=$(($i*3+1))
        for ((j=2;j<=32;j=j*2)); do
            if [ "$protocol" == "archipelago" ]; then
                python generate_config.py --file_prefix=config/test --protocol=$protocol --client_batchsize=$beta --number_operations=$number_operations --commit_duration=$commit_duration $numservers $numservers $numclientservers $j
            else
                python generate_config.py --file_prefix=config/test --protocol=$protocol --replica_batchsize=$beta --number_operations=$number_operations $numservers $numservers $numclientservers $j
            fi
            python run.py config/test_servers_$numservers\_clients_$j\_$protocol.json
            python analyze.py latest_rslt/config.json
	    dest=$EXPDIR/server_inc_servers_$numservers\_clients_$j\_beta_$beta\_commit_$commit_duration.json
            cp latest_rslt/rslt.json $dest
        done
    done
elif [ "$1" == "server_inc_result" ]; then
    echo "Calculate $protocol server increase experiment result"
    rm -rf $EXPDIR/*_beta_$beta\_*
    for i in 1 2 3 4 5 10; do
        numservers=$(($i*3+1))
        best=''
        best_throughput=0
        best_latency=100000.0
        for ((j=2;j<=128;j=j*2)); do
	    dest=$FOLDER/$protocol/$4/server_inc_servers_$numservers\_clients_$j\_beta_$beta\_commit_$commit_duration.json
	    if [ ! -f "$dest" ]; then
                continue
            fi

            throughput=`grep \"throughput\" $dest | awk '{print $2;}'`
            latency=`grep \"50_totalorder_latency\" $dest | awk '{split($2,s,",");print s[1];}'`

            latency_td=$(echo "$latency < $best_latency*2.0" | bc -l)
            throughput_td=$(echo "$throughput > $best_throughput*1.1" | bc -l)

            ordering_latency=`grep \"50_latency\" $dest | awk '{split($2,s,",");print s[1];}'`
            echo "s_$numservers\_c_$j: throughput: $throughput latency:$latency ordering_latency:$ordering_latency throughput_td:$throughput_td latency_td:$latency_td"

            if [ "$throughput_td" == "1" ] && [ "$latency_td" == "1" ]; then
                best_throughput=$throughput
                best_latency=$latency
                best=$dest
            fi
        done
        echo "best: $best"
        grep \"throughput\" $best
        grep \"50_latency\" $best
        grep \"50_totalorder_latency\" $best
        cp $best $EXPDIR/
    done
elif [ "$1" == "client_inc_result" ]; then
    echo "Calculate $protocol client increase experiment result"
    grep \"throughput\" $EXPDIR -R
    grep \"50_latency\" $EXPDIR -R
    grep \"50_totalorder_latency\" $EXPDIR -R
elif [ "$1" == "single" ]; then
    numservers=$4
    numclients=$5
    exp=$6
    if [ "$protocol" == "archipelago" ]; then
        python generate_config.py --file_prefix=config/test --protocol=$protocol --client_batchsize=$beta --number_operations=$number_operations --commit_duration=$commit_duration $numservers $numservers $numclientservers $numclients
    else
        python generate_config.py --file_prefix=config/test --protocol=$protocol --replica_batchsize=$beta --number_operations=$number_operations $numservers $numservers $numclientservers $numclients
    fi
    python run.py config/test_servers_$numservers\_clients_$numclients\_$protocol.json
    python analyze.py latest_rslt/config.json
    dest=$FOLDER/$protocol/$exp/$exp\_servers_$numservers\_clients_$numclients\_beta_$beta\_commit_$commit_duration.json
    cp latest_rslt/rslt.json $dest
fi
