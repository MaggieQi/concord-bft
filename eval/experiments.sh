FOLDER=~/archipelago/paper/rslt
protocol=$2
mkdir -p $FOLDER/$protocol
mkdir config
if [ "$1" == "client_inc" ]; then
    echo "RUN $protocol client increase experiment"
    for ((i=1;i<512;i=i*2)); do
        python generate_config.py --file_prefix=config/test --protocol=$protocol --fresh_install=False 4 4 16 $i
        python run.py config/test_servers_4_clients_$i\_$protocol.json
        python analyze.py latest_rslt/config.json
        cp latest_rslt/rslt.json $FOLDER/$protocol/client_inc_servers_4_clients_$i.json
    done
elif [ "$1" == "server_inc" ]; then
    echo "RUN $protocol server increase experiment"
    for ((i=1;i<=5;i=i+1)); do
        numservers=$(($i*3+1))
        python generate_config.py --file_prefix=config/test --protocol=$protocol --fresh_install=False $numservers $numservers 4 64
        python run.py config/test_servers_$numservers\_clients_64_$protocol.json
        python analyze.py latest_rslt/config.json
        cp latest_rslt/rslt.json $FOLDER/$protocol/server_inc_servers_$numservers\_clients_64.json
    done
fi

