Experiment Setup
----
Prepare a cluster of machines (virtual machines) which can be connected to each other without ssh password.

Driver Setup
----
Driver machine is to coordinate all the processes running in the servers and clients. Use the following commands to prepare the driver environment.
```bash
#!/bin/bash
sudo apt-get update
sudo apt-get install -y python-pip python-dev build-essential git
sudo pip install --upgrade pip
sudo pip install --upgrade virtualenv
sudo pip install paramiko numpy
git clone https://github.com/MaggieQi/concord-bft
cd concord-bft
git checkout add_archipelago
```

Servers and Clients Setup
----
To initialize the experiment environment in servers and clients, first modify the generate_config.py in the eval folder in the driver machine to include correct ip addresses for servers and clients:
```python
import sys

def generate_config(file_prefix, algo, num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, fresh_install):
    content = '''{
    "num_servers": %s,
    "servers": {
        "0": "10.0.0.5",
        "1": "10.0.0.6",
        "2": "10.0.0.7",
        "3": "10.0.0.8"
    },
    "server_resources": {
        "0": 4,
        "1": 4,
        "2": 4,
        "3": 4,
        "4": 4
    },
    "num_replicas": %s,
    "num_clients": %s,
    "clients": {
        "0": "10.0.0.9",
        "1": "10.0.0.10",
        "2": "10.0.0.11",
        "3": "10.0.0.12"
    },
    "client_resources": {
        "0": 4,
        "1": 4,
        "2": 4,
        "3": 4
    },
    "num_client_threads": %s,
    "do_fresh_install": %s,
    "system" : "%s",
    "exp_name": "arch_throughput",
    "network": "tcp"
}
    ''' % (num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, fresh_install, algo)

    f = open("%s_servers_%s_clients_%s_%s.json" % (file_prefix, num_replica_threads, num_client_threads, algo), 'w')
    f.write(content)
    f.close()

if __name__ == '__main__':
    if len(sys.argv) < 8:
        print ("Usage: generate_config.py file_prefix algo num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, fresh_install")
        exit(1)
```

Then run the following commands in the driver machine (replace the \<number of servers\> \<number of replica threads\> \<number of clients\> \<number of client threads\> with the real number).
```bash
python generate_config.py init concord <number of servers> <number of replica threads> <number of clients> <number of client threads> true
python run.py init_servers_<number of servers>_clients_<number of clients>_concord.json init
```
It will install all the required dependencies and the concord-bft in all the servers and clients.

Run experiments
----

