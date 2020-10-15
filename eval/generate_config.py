import sys
from optparse import OptionParser
import json

parser = OptionParser(usage="Usage: generate_config.py <options> num_replica_servers num_replica_threads num_client_servers num_client_threads")
parser.add_option("-d", "--commit_duration", default=50, type="int", help="Pompe consensus commit duration.")
parser.add_option("-i", "--number_operations", default=2800, type="int", help="Number of operations each client sends.")
parser.add_option("-r", "--replica_batchsize", default=1, type="int", help="Server max batch size.")
parser.add_option("-c", "--client_batchsize", default=1, type="int", help="Client max batch size.")
parser.add_option("-f", "--fresh_install", default=False, action='store_true', help="Reinstall dependencies and concord-bft code.")
parser.add_option("-a", "--protocol", default="pompe", type="string", help="run protocol concord or pompe.")
parser.add_option('-p', "--file_prefix", default="test", type="string", help="configure file path prefix.")
parser.add_option('-e', "--env", default="geo", type="string", help="local or geo.")
parser.add_option("-t", "--threads", default=8, type="int", help="Working threads in the replica.")

local_server_list = "10.0.2.7,10.0.2.8,10.0.2.9,10.0.2.10,10.0.2.11,10.0.2.12,10.0.2.13,10.0.2.14,10.0.2.15,10.0.2.16,10.0.2.17,10.0.2.18,10.0.2.19,10.0.2.20,10.0.2.21,10.0.2.22,10.0.2.23,10.0.2.24,10.0.2.25,10.0.2.26,10.0.2.27,10.0.2.28,10.0.2.29,10.0.2.30,10.0.2.31,10.0.2.32,10.0.2.33,10.0.2.34,10.0.2.35,10.0.2.36,10.0.2.37"
local_client_list = "10.0.2.37,10.0.2.36,10.0.2.35,10.0.2.34,10.0.2.33,10.0.2.32,10.0.2.31,10.0.2.30,10.0.2.29,10.0.2.28,10.0.2.27,10.0.2.26,10.0.2.25,10.0.2.24,10.0.2.23,10.0.2.22,10.0.2.21,10.0.2.20,10.0.2.19,10.0.2.18,10.0.2.17,10.0.2.16,10.0.2.15,10.0.2.14,10.0.2.13,10.0.2.12,10.0.2.11"

geo_server_list = "10.0.2.7,10.0.3.6,10.0.4.6,10.0.2.8,10.0.3.7,10.0.4.7,10.0.2.9,10.0.3.8,10.0.4.8,10.0.2.10,10.0.3.9,10.0.4.9,10.0.2.11,10.0.3.10,10.0.4.10,10.0.2.12,10.0.3.11,10.0.4.11,10.0.2.13,10.0.3.12,10.0.4.12,10.0.2.14,10.0.3.13,10.0.4.13,10.0.2.15,10.0.3.14,10.0.4.14,10.0.2.16,10.0.3.15,10.0.4.15,10.0.2.17,10.0.3.16,10.0.4.16,10.0.2.18,10.0.3.17,10.0.4.17,10.0.2.19,10.0.3.18,10.0.4.18,10.0.2.20,10.0.3.19,10.0.4.19,10.0.2.21,10.0.3.20,10.0.4.20,10.0.2.22,10.0.3.21,10.0.4.21,10.0.2.23,10.0.3.22,10.0.4.22,10.0.2.24,10.0.3.23,10.0.4.23,10.0.2.25,10.0.3.24,10.0.4.24,10.0.2.26,10.0.3.25,10.0.4.25,10.0.2.27,10.0.3.26,10.0.4.26,10.0.2.28,10.0.3.27,10.0.4.27,10.0.2.29,10.0.3.28,10.0.4.28,10.0.2.30,10.0.3.29,10.0.4.29,10.0.2.31,10.0.3.30,10.0.4.30,10.0.2.32,10.0.3.31,10.0.4.31,10.0.2.33,10.0.3.32,10.0.4.32,10.0.2.34,10.0.3.33,10.0.4.33,10.0.2.35,10.0.3.34,10.0.4.34,10.0.2.36,10.0.3.35,10.0.4.35,10.0.2.37,10.0.3.36,10.0.4.36,10.0.2.38,10.0.3.37,10.0.4.37,10.0.2.39,10.0.3.38,10.0.4.38,10.0.2.6"
geo_client_list = "10.0.2.7,10.0.3.6,10.0.4.6,10.0.2.8,10.0.3.7,10.0.4.7,10.0.2.9,10.0.3.8,10.0.4.8,10.0.2.10,10.0.3.9,10.0.4.9,10.0.2.11,10.0.3.10,10.0.4.10,10.0.2.12,10.0.3.12,10.0.4.11,10.0.2.13,10.0.3.12,10.0.4.12,10.0.2.14,10.0.3.13,10.0.4.13,10.0.2.15,10.0.3.14,10.0.4.14,10.0.2.16,10.0.3.15,10.0.4.15,10.0.2.17,10.0.3.16,10.0.4.16,10.0.2.18,10.0.3.17,10.0.4.17,10.0.2.19,10.0.3.18,10.0.4.18,10.0.2.20,10.0.3.19,10.0.4.19,10.0.2.21,10.0.3.20,10.0.4.20,10.0.2.22,10.0.3.21,10.0.4.21,10.0.2.23,10.0.3.22,10.0.4.22,10.0.2.24,10.0.3.23,10.0.4.23,10.0.2.25,10.0.3.24,10.0.4.24,10.0.2.26,10.0.3.25,10.0.4.25,10.0.2.27,10.0.3.26,10.0.4.26,10.0.2.28,10.0.3.27,10.0.4.27,10.0.2.29,10.0.3.28,10.0.4.28,10.0.2.30,10.0.3.29,10.0.4.29,10.0.2.31,10.0.3.30,10.0.4.30,10.0.2.32,10.0.3.31,10.0.4.31,10.0.2.33,10.0.3.32,10.0.4.32,10.0.2.34,10.0.3.33,10.0.4.33,10.0.2.35,10.0.3.34,10.0.4.34,10.0.2.36,10.0.3.35,10.0.4.35,10.0.2.37,10.0.3.36,10.0.4.36,10.0.2.38,10.0.3.37,10.0.4.37,10.0.2.39,10.0.3.38,10.0.4.38"

def generate_machinelist(l):
    mlist = ''
    rlist = ''
    for i, m in enumerate(l.split(",")):
        mlist += '        \"%d\": \"%s\",\n' % (i, m)
        rlist += '        \"%d\": 1,\n' % i
    return mlist[0:-2], rlist[0:-2]

def generate_config(num_replica_servers, num_replica_threads, num_client_servers, num_client_threads, options):
    global local_server_list, local_client_list, geo_server_list, geo_client_list
    server_machines, server_resources = generate_machinelist(geo_server_list if options.env == "geo" else local_server_list)
    client_machines, client_resources = generate_machinelist(geo_client_list if options.env == "geo" else local_client_list)
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
    "client_batchsize": %d,
    "env": "%s",
    "threads": %d
}
''' % (num_replica_servers, server_machines, server_resources, num_replica_threads, num_client_servers, client_machines, client_resources, num_client_threads, json.dumps(options.fresh_install), options.protocol, options.commit_duration, options.number_operations, options.replica_batchsize, options.client_batchsize, options.env, options.threads)

    f = open("%s_servers_%s_clients_%s_%s.json" % (options.file_prefix, num_replica_threads, num_client_threads, options.protocol), 'w')
    f.write(content)
    f.close()

if __name__ == '__main__':
    if len(sys.argv) < 5:
        parser.print_help()
        exit(1)

    (options, args) = parser.parse_args()
    generate_config(args[0], args[1], args[2], args[3], options)
