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

server_list = "10.0.2.7,10.0.2.8,10.0.2.9,10.0.2.10,10.0.2.11,10.0.2.12,10.0.2.13,10.0.2.14,10.0.2.15,10.0.2.16,10.0.2.17,10.0.2.18,10.0.2.19,10.0.2.20,10.0.2.21,10.0.2.22,10.0.2.23,10.0.2.24,10.0.2.25,10.0.2.26,10.0.2.27,10.0.2.28,10.0.2.29,10.0.2.30,10.0.2.31,10.0.2.32,10.0.2.33,10.0.2.34,10.0.2.35,10.0.2.36,10.0.2.37"
client_list = "10.0.2.37,10.0.2.36,10.0.2.35,10.0.2.34,10.0.2.33,10.0.2.32,10.0.2.31,10.0.2.30,10.0.2.29,10.0.2.28,10.0.2.27,10.0.2.26,10.0.2.25,10.0.2.24,10.0.2.23,10.0.2.22,10.0.2.21,10.0.2.20,10.0.2.19,10.0.2.18,10.0.2.17,10.0.2.16,10.0.2.15,10.0.2.14,10.0.2.13,10.0.2.12,10.0.2.11,10.0.2.10,10.0.2.9,10.0.2.8,10.0.2.7"
#server_list = "10.0.2.7,10.0.2.8,10.0.2.9,10.0.2.10"
#client_list = "10.0.2.7,10.0.2.8,10.0.2.9,10.0.2.10"

def generate_machinelist(l):
    mlist = ''
    rlist = ''
    for i, m in enumerate(l.split(",")):
        mlist += '        \"%d\": \"%s\",\n' % (i, m)
        rlist += '        \"%d\": 1,\n' % i
    return mlist[0:-2], rlist[0:-2]

def generate_config(num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, options):
    global server_list, client_list
    server_machines, server_resources = generate_machinelist(server_list)
    client_machines, client_resources = generate_machinelist(client_list)
    content = '''{
    "num_servers": %s,
    "servers": {
%s
    },
    "server_resources": {
%s
    },
    "num_replicas": %s,
    "num_clients": %s,
    "clients": {
%s
    },
    "client_resources": {
%s
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
''' % (num_replica_servers, server_machines, server_resources, num_replica_threads, num_client_servers, client_machines, client_resources, num_client_threads, json.dumps(options.fresh_install), options.protocol, options.commit_duration, options.number_operations, options.replica_batchsize, options.client_batchsize)

    f = open("%s_servers_%s_clients_%s_%s.json" % (options.file_prefix, num_replica_threads, num_client_threads, options.protocol), 'w')
    f.write(content)
    f.close()

if __name__ == '__main__':
    if len(sys.argv) < 5:
        parser.print_help()
        exit(1)

    (options, args) = parser.parse_args()
    generate_config(args[0], args[1], args[2], args[3], options)
