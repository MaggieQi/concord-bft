# Pompe-C

This repo contains our implementation of Pompe on top of Concord.

Experiment Setup
----
Prepare a cluster of machines (virtual machines) which can be connected to each other without ssh password.

Driver Setup
----
Driver machine is to coordinate all the processes running in the servers and clients. Use the following commands to prepare the driver environment.
```bash
#!/bin/bash
sudo apt-get update
sudo apt-get install -y python3-pip python3-dev python3-tk build-essential git rsync
sudo pip3 install --upgrade pip
sudo pip3 install --upgrade virtualenv
sudo pip3 install paramiko numpy matplotlib==2.2.4
git clone https://github.com/MaggieQi/concord-bft
cd concord-bft
git checkout add_multiple_listenthreads
```

Servers and Clients Setup
----
To initialize the experiment environment in servers and clients, first modify the generate_config.py in the eval folder in the driver machine to include correct ip addresses for servers and clients:
```python
local_server_list = "10.0.2.7,10.0.2.8,10.0.2.9,10.0.2.10,10.0.2.11,10.0.2.12,10.0.2.13,10.0.2.14,10.0.2.15,10.0.2.16,10.0.2.17,10.0.2.18,10.0.2.19,10.0.2.20,10.0.2.21,10.0.2.22,10.0.2.23,10.0.2.24,10.0.2.25,10.0.2.26,10.0.2.27,10.0.2.28,10.0.2.29,10.0.2.30,10.0.2.31,10.0.2.32,10.0.2.33,10.0.2.34,10.0.2.35,10.0.2.36,10.0.2.37"
local_client_list = "10.0.2.37,10.0.2.36,10.0.2.35,10.0.2.34,10.0.2.33,10.0.2.32,10.0.2.31,10.0.2.30,10.0.2.29,10.0.2.28,10.0.2.27,10.0.2.26,10.0.2.25,10.0.2.24,10.0.2.23,10.0.2.22,10.0.2.21,10.0.2.20,10.0.2.19,10.0.2.18,10.0.2.17,10.0.2.16,10.0.2.15,10.0.2.14,10.0.2.13,10.0.2.12,10.0.2.11"

geo_server_list = "10.0.2.7,10.0.3.6,10.0.4.6,10.0.2.8,10.0.3.7,10.0.4.7,10.0.2.9,10.0.3.8,10.0.4.8,10.0.2.10,10.0.3.9,10.0.4.9,10.0.2.11,10.0.3.10,10.0.4.10,10.0.2.12,10.0.3.11,10.0.4.11,10.0.2.13,10.0.3.12,10.0.4.12,10.0.2.14,10.0.3.13,10.0.4.13,10.0.2.15,10.0.3.14,10.0.4.14,10.0.2.16,10.0.3.15,10.0.4.15,10.0.2.17,10.0.3.16,10.0.4.16,10.0.2.18,10.0.3.17,10.0.4.17,10.0.2.19,10.0.3.18,10.0.4.18,10.0.2.20,10.0.3.19,10.0.4.19,10.0.2.21,10.0.3.20,10.0.4.20,10.0.2.22,10.0.3.21,10.0.4.21,10.0.2.23,10.0.3.22,10.0.4.22,10.0.2.24,10.0.3.23,10.0.4.23,10.0.2.25,10.0.3.24,10.0.4.24,10.0.2.26,10.0.3.25,10.0.4.25,10.0.2.27,10.0.3.26,10.0.4.26,10.0.2.28,10.0.3.27,10.0.4.27,10.0.2.29,10.0.3.28,10.0.4.28,10.0.2.30,10.0.3.29,10.0.4.29,10.0.2.31,10.0.3.30,10.0.4.30,10.0.2.32,10.0.3.31,10.0.4.31,10.0.2.33,10.0.3.32,10.0.4.32,10.0.2.34,10.0.3.33,10.0.4.33,10.0.2.35,10.0.3.34,10.0.4.34,10.0.2.36,10.0.3.35,10.0.4.35,10.0.2.37,10.0.3.36,10.0.4.36,10.0.2.38,10.0.3.37,10.0.4.37,10.0.2.39,10.0.3.38,10.0.4.38,10.0.2.6"
geo_client_list = "10.0.2.7,10.0.3.6,10.0.4.6,10.0.2.8,10.0.3.7,10.0.4.7,10.0.2.9,10.0.3.8,10.0.4.8,10.0.2.10,10.0.3.9,10.0.4.9,10.0.2.11,10.0.3.10,10.0.4.10,10.0.2.12,10.0.3.12,10.0.4.11,10.0.2.13,10.0.3.12,10.0.4.12,10.0.2.14,10.0.3.13,10.0.4.13,10.0.2.15,10.0.3.14,10.0.4.14,10.0.2.16,10.0.3.15,10.0.4.15,10.0.2.17,10.0.3.16,10.0.4.16,10.0.2.18,10.0.3.17,10.0.4.17,10.0.2.19,10.0.3.18,10.0.4.18,10.0.2.20,10.0.3.19,10.0.4.19,10.0.2.21,10.0.3.20,10.0.4.20,10.0.2.22,10.0.3.21,10.0.4.21,10.0.2.23,10.0.3.22,10.0.4.22,10.0.2.24,10.0.3.23,10.0.4.23,10.0.2.25,10.0.3.24,10.0.4.24,10.0.2.26,10.0.3.25,10.0.4.25,10.0.2.27,10.0.3.26,10.0.4.26,10.0.2.28,10.0.3.27,10.0.4.27,10.0.2.29,10.0.3.28,10.0.4.28,10.0.2.30,10.0.3.29,10.0.4.29,10.0.2.31,10.0.3.30,10.0.4.30,10.0.2.32,10.0.3.31,10.0.4.31,10.0.2.33,10.0.3.32,10.0.4.32,10.0.2.34,10.0.3.33,10.0.4.33,10.0.2.35,10.0.3.34,10.0.4.34,10.0.2.36,10.0.3.35,10.0.4.35,10.0.2.37,10.0.3.36,10.0.4.36,10.0.2.38,10.0.3.37,10.0.4.37,10.0.2.39,10.0.3.38,10.0.4.38"
```

Then run the following commands in the driver machine (replace the \<env\> with local or geo to choose correct server and client lists, replace \<number of servers\> \<number of replica threads\> \<number of clients\> \<number of client threads\> with the real number of servers and clients).
```bash
python3 generate_config.py --file_prefix=init --env=<env> --protocol=concord --fresh_install <number of servers> <number of replica threads> <number of clients> <number of client threads>
python3 run.py init_servers_<number of replica threads>_clients_<number of client threads>_concord.json init
```
It will install all the required dependencies and the concord-bft in all the servers and clients.

Run experiments
----
* 6.1 End-to-end performance: Throughput and latency

Peak throughput and median latency for a system with n = 4 nodes. Vary beta to achieve the comparable consensus latency with pompe (beta=1). Here we use 800 for concord and 200 for pompe.

```bash
./experiments.sh client_inc concord 1 50 local         # step1.1
./experiments.sh client_inc pompe 1 50 local           # step1.2
./experiments.sh client_inc concord <beta> 50 local    # step1.3
./experiments.sh client_inc pompe <beta> 50 local      # step1.4
./experiments.sh client_inc_result concord <beta> 50 local client_inc # check concord peak results for different beta
./experiments.sh client_inc_result pompe <beta> 50 local client_inc   # check pompe peak results for different beta

./experiments.sh client_inc concord <beta> 500 geo     # step1.5
./experiments.sh client_inc pompe <beta> 500 geo       # step1.6
./experiments.sh client_inc_result concord <beta> 500 geo client_inc  # check concord peak results for different beta
./experiments.sh client_inc_result pompe <beta> 500 geo client_inc    # check pompe peak results for different beta
```
Note the results will contain throughput, 50_latency (ordering latency) and 50_totalorder_latency (ordering + consensus latency).

### Validate claim1

> :warning: The cloud instance we used for our submission were deleted by Azure. We are using D16s_v3 instances for revision now and here are the numbers on this new cloud instance. They are similar but not completely the same as our Figure 4 in the submission.


| env |                             | throughput (cmd/s) | latency (ms)       | validation |
|-----|-----------------------------|--------------------|--------------------|------------|
|local| Concord (beta = 1)          |        40.0        |      53.0          | step1.1    |
|     | Concord (beta = 800)        |      6633.0        |      67.0          | step1.3    |
|     | Pompe-C (beta = 1)          |      1415.0        | 17.0(o) 67.0\(c\)  | step1.2    |
|     | Pompe-C (beta = 200)        |    249221.0        | 18.0(o) 74.0\(c\)  | step1.4    |
| geo | Concord (beta = 800)        |      1461.0        |      616.0         | step1.5    |
|     | Pompe-C (beta = 200)        |    172774.0        |325.0(o) 1415.0\(c\)| step1.6    |
In claim1, we say that Pompe achieves higher throughput than baseline (1415.0 > 40.0; 249221.0 > 6633.0) at the cost of increased latency (67.0 > 53.0). But latencies are comparable (67.0 vs. 67.0) when batching commands (beta is the batch size).

* 6.2 Scalability

Peak throughput and median latency with varying number of nodes (from 4 to 100).
```bash
./experiments.sh server_inc concord 800 500 geo 0
./experiments.sh server_inc pompe 200 500 geo 0 # fix beta to 200
./experiments.sh server_inc pompe 200 500 geo 1 # enable beta decrease
./experiments.sh server_inc_result concord 800 500 geo 0 server_inc
./experiments.sh server_inc_result pompe 200 500 geo 0 server_inc
./experiments.sh server_inc_result pompe 200 500 geo 1 server_inc
python plot.py ~/pompe/paper/rslt server_inc_result .
```
### Validate claim2

We conduct the expriments of Figure 6 in our new cloud instance and below is the raw data. The plot of this table looks basically the same as Figure 6 in our submission.

|        |  <td colspan=2>concord beta = 800         | <td colspan=2>pompe beta = 200                          | <td colspan=2>pompe beta = 800 / n                      |
|#machine| throughput (cmd/s)  | median latency (ms) | throughput (cmd/s)       | median ordering latency (ms) | throughput (cmd/s)       | median ordering latency (ms) |
|--------|---------------------|---------------------|--------------------------|------------------------------|--------------------------|------------------------------
| n=4    |  1466.0             |        634.0        |     172774.0             |             325.0            |     172774.0             |             325.0            |
| n=10   |  1515.0             |        728.0        |     121466.0             |             557.0            |     39773.0              |             554.0            |
| n=16   |  1540.0             |        787.0        |     72400.0              |             836.0            |     18250.0              |             673.0            |
| n=31   |  902.0              |        977.0        |     37538.0              |             803.0            |     4624.0               |             681.0            |
| n=61   |  906.0              |        1011.0       |     21789.0              |             826.0            |     1365.0               |             875.0            | 
| n=100  |  896.0              |        1028.0       |     12882.0              |             970.0            |     524.0                |             951.0            |

Note that the baseline uses BLS threshold signature to reduce the total computational cost of verifying signatures. Therefore, the verification cost will not increase much when the number of servers increases. Pompe-C, Hotstuff and Pompe-Hotstuff use other signature verification method (since the timestamps from different replicas that need to be verified are different), therefore the timestamp verification cost will increase linearly with the number of servers.
