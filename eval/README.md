# Archipelago-C

This repo contains our implementation of Archipelago on top of Concord.

Experiment Setup
----
Prepare a cluster of machines (virtual machines) which can be connected to each other without ssh password.

Driver Setup
----
Driver machine is to coordinate all the processes running in the servers and clients. Use the following commands to prepare the driver environment.
```bash
#!/bin/bash
sudo apt-get update
sudo apt-get install -y python-pip python-dev python-tk build-essential git
sudo pip install --upgrade pip
sudo pip install --upgrade virtualenv
sudo pip install paramiko numpy matplotlib==2.2.4
git clone https://github.com/MaggieQi/concord-bft
cd concord-bft
git checkout add_archipelago
```

Servers and Clients Setup
----
To initialize the experiment environment in servers and clients, first modify the generate_config.py in the eval folder in the driver machine to include correct ip addresses for servers and clients:
```python
import sys
from optparse import OptionParser
import json

parser = OptionParser(usage="Usage: generate_config.py <options> num_replica_servers num_replica_threads num_client_servers num_client_threads")
parser.add_option("-d", "--commit_duration", default=100, type="int", help="Archipelago consensus commit duration.")
parser.add_option("-i", "--number_operations", default=2800, type="int", help="Number of operations each client sends.")
parser.add_option("-r", "--replica_batchsize", default=1, type="int", help="Server max batch size.")
parser.add_option("-c", "--client_batchsize", default=1, type="int", help="Client max batch size.")
parser.add_option("-f", "--fresh_install", default=False, action='store_true', help="Reinstall dependencies and concord-bft code.")
parser.add_option("-a", "--protocol", default="archipelago", type="string", help="run protocol concord or archipelago.")
parser.add_option('-p', "--file_prefix", default="test", type="string", help="configure file path prefix.")

def generate_config(num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, options):
    content = '''{
    "num_servers": %s,
    "servers": {
        "0": "10.0.0.5",
        "1": "10.0.0.6",
        "2": "10.0.0.7",
        "3": "10.0.0.8",
        "4": "10.0.0.9",
        "5": "10.0.0.10",
        "6": "10.0.0.11",
        "7": "10.0.0.12",
        "8": "10.0.0.13",
        "9": "10.0.0.14",
        "10": "10.0.0.15",
        "11": "10.0.0.16",
        "12": "10.0.0.17",
        "13": "10.0.0.18",
        "14": "10.0.0.19",
        "15": "10.0.0.20"
    },
    "server_resources": {
        "0": 4,
        "1": 4,
        "2": 4,
        "3": 4,
        "4": 4,
        "5": 4,
        "6": 4,
        "7": 4,
        "8": 4,
        "9": 4,
        "10": 4,
        "11": 4,
        "12": 4,
        "13": 4,
        "14": 4,
        "15": 4
    },
    "num_replicas": %s,
    "num_clients": %s,
    "clients": {
        "0": "10.0.0.24",
        "1": "10.0.0.23",
        "2": "10.0.0.22",
        "3": "10.0.0.21",
        "4": "10.0.0.20",
        "5": "10.0.0.19",
        "6": "10.0.0.18",
        "7": "10.0.0.17",
        "8": "10.0.0.16",
        "9": "10.0.0.15",
        "10": "10.0.0.14",
        "11": "10.0.0.13",
        "12": "10.0.0.12",
        "13": "10.0.0.11",
        "14": "10.0.0.10",
        "15": "10.0.0.9"
    },
    "client_resources": {
        "0": 4,
        "1": 4,
        "2": 4,
        "3": 4,
        "4": 4,
        "5": 4,
        "6": 4,
        "7": 4,
        "8": 4,
        "9": 4,
        "10": 4,
        "11": 4,
        "12": 4,
        "13": 4,
        "14": 4,
        "15": 4
    },
    "num_client_threads": %s,
    "do_fresh_install": %s,
    "system" : "%s",
    "exp_name": "arch_throughput",
    "network": "tcp",
    "commit_duration": %d,
    "number_operations": %d,
    "replica_batchsize": %d,
    "client_batchsize": %d
}
''' % (num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, json.dumps(options.fresh_install), options.protocol, options.commit_duration, options.number_operations, options.replica_batchsize, options.client_batchsize)

    f = open("%s_servers_%s_clients_%s_%s.json" % (options.file_prefix, num_replica_threads, num_client_threads, options.protocol), 'w')
    f.write(content)
    f.close()

if __name__ == '__main__':
    if len(sys.argv) < 5:
        parser.print_help()
        exit(1)

    (options, args) = parser.parse_args()
    generate_config(args[0], args[1], args[2], args[3], options)
```

Then run the following commands in the driver machine (replace the \<number of servers\> \<number of replica threads\> \<number of clients\> \<number of client threads\> with the real number).
```bash
python generate_config.py --file_prefix=init --protocol=concord --fresh_install <number of servers> <number of replica threads> <number of clients> <number of client threads>
python run.py init_servers_<number of replica threads>_clients_<number of client threads>_concord.json init
```
It will install all the required dependencies and the concord-bft in all the servers and clients.

Run experiments
----
* 6.1 End-to-end performance: Throughput and latency

Peak throughput and median latency for a system with n = 4 nodes. Vary beta to achieve the comparable consensus latency with archipelago (beta=1).
```bash
./experiment.sh client_inc concord 1             # step1.1
./experiment.sh client_inc archipelago 1         # step1.2
./experiment.sh client_inc concord <beta>        # step1.3
./experiment.sh client_inc archipelago <beta>    # step1.4
./experiment.sh client_inc_result concord        # check concord peak results for different beta
./experiment.sh client_inc_result archipelago    # check archipelago peak results for different beta
```
Note the results will contain throughput, 50_latency (ordering latency) and 50_totalorder_latency (ordering + consensus latency).

### Validate claim1

> :warning: The cloud instance we used for our submission were deleted by Azure. We are using a similar instance for revision now and here are the numbers on this new cloud instance. They are similar but not completely the same as our Figure 4 in the submission. 


|                             | throughput (cmd/s) | latency (ms)       | validation |
|-----------------------------|--------------------|--------------------|------------|
| Concord (beta = 1)          |        43.0        |      48.0          | step1.1    |
| Concord (beta = 1500)       |      6322.0        |      109.0         | step1.3    |
| Archipelago-C (beta = 1)    |       315.0        | 11.0(o) 108.0\(c\) | step1.2    |
| Archipelago-C (beta = 200)  |     54127.0        | 9.0(o) 92.0\(c\)   | step1.4    |

In claim1, we say that Archipelago achieves higher throughput than baseline (315.0 > 43.0; 54127.0 > 6322.0) at the cost of increased latency (108.0 > 48.0). But latencies are comparable (109.0 vs. 108.0) when batching commands (beta is the batch size).

* 6.2 Scalability

Peak throughput and median latency with varying number of nodes (from 4 to 16) and beta=1.
```bash
./experiment.sh server_inc concord
./experiment.sh server_inc archipelago
./experiment.sh server_inc_result concord
./experiment.sh server_inc_result archipelago
python plot.py ~/archipelago/paper/rslt server_inc_result .
```
### Validate claim2

We conduct the expriments of Figure 6 in our new cloud instance and below is the raw data. The plot of this table looks basically the same as Figure 6 in our submission.

|      | baseline throughput (cmd/s) | median latency (ms) | Archipelago throughput (cmd/s) | median ordering latency (ms) |
|------|-----------------------------|---------------------|--------------------------------|------------------------------|
| n=4  |          45.0               |        47.0         |           320.0                |             12.0             |
| n=7  |          42.0               |        47.0         |           177.0                |             10.0             |
| n=10 |          41.0               |        49.0         |           126.0                |             14.0             |
| n=13 |          42.0               |        49.0         |           101.0                |             18.0             |
| n=16 |          41.0               |        50.0         |           81.0                 |             23.0             |

Note that the baseline uses BLS threshold signature to reduce the total computational cost of verifying signatures. Therefore, the verification cost will not increase much when the number of servers increases. Archipelago-C, Hotstuff and Archipelago-Hotstuff use other signature verification method (since the timestamps from different replicas that need to be verified are different), therefore the timestamp verification cost will increase linearly with the number of servers.

