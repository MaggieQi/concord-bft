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
