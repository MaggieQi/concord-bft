FOLDER=~/archipelago/paper/rslt
protocol=$2
EXPDIR=$FOLDER/$protocol/$1
number_operations=2800
commit_duration=100
beta=1
if [ "$3" != "" ]; then
    beta=$3
    if [ "$beta" != "1" ]; then
        number_operations=$((50*$beta))
    fi
    echo "Set batch size: $beta number of operations: $number_operations"
fi

mkdir -p $EXPDIR
mkdir config
if [ "$1" == "client_inc" ]; then
    echo "RUN $protocol client increase experiment"
    best=''
    best_throughput=0.0
    best_latency=10000.0
    for ((i=2;i<512;i=i*2)); do
        if [ "$protocol" == "archipelago" ]; then
            python generate_config.py --file_prefix=config/test --protocol=$protocol --client_batchsize=$beta --number_operations=$number_operations --commit_duration=$commit_duration 4 4 16 $i
        else
            python generate_config.py --file_prefix=config/test --protocol=$protocol --replica_batchsize=$beta --number_operations=$number_operations 4 4 16 $i
        fi
        python run.py config/test_servers_4_clients_$i\_$protocol.json
        python analyze.py latest_rslt/config.json
        dest=$EXPDIR/client_inc_servers_4_clients_$i\_beta_$beta\_commit_$commit_duration.json
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
    for ((i=1;i<=5;i=i+1)); do
        numservers=$(($i*3+1))
        for ((j=2;j<=8;j=j*2)); do
            if [ "$protocol" == "archipelago" ]; then
                python generate_config.py --file_prefix=config/test --protocol=$protocol --client_batchsize=$beta --number_operations=$number_operations --commit_duration=$commit_duration $numservers $numservers 4 $j
            else
                python generate_config.py --file_prefix=config/test --protocol=$protocol --replica_batchsize=$beta --number_operations=$number_operations $numservers $numservers 4 $j
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
    for ((i=1;i<=5;i=i+1)); do
        numservers=$(($i*3+1))
        best=''
        best_throughput=0
        for ((j=2;j<=8;j=j*2)); do
	    dest=$FOLDER/$protocol/server_inc/server_inc_servers_$numservers\_clients_$j\_beta_$beta\_commit_$commit_duration.json 
            throughput=`grep \"throughput\" $dest | awk '{print $2;}'`
            threshold=$(echo $best_throughput*1.1 | bc)
            #echo servers_$numservers\_clients_$j:$threshold $throughput
            if (( $(echo "$throughput > $threshold" | bc -l) )); then
                best_throughput=$throughput
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
fi
