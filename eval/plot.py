import sys
import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import json

colors = ['olive', 'midnightblue', 'firebrick', 'chocolate', 'c', 'p', 'y']
markers = ['o', 's', 'v', '^', 'X', 'P', 'D']

def plot(folder, results, prefix):
    fig, (ax0, ax1, ax2) = plt.subplots(nrows = 3)

    if prefix.startswith('server_inc'):
        xkey = 'num_replicas'
        xlabel = '#servers'
    else:
        xkey = 'num_client_threads'
        xlabel = '#clients'

    ax0.set_xlabel(xlabel)
    ax0.set_ylabel('throughput')
    longest = []
    for i, key in enumerate(results):
        ilabel = results[key][xkey]
        ax0.plot(np.arange(len(results[key][xkey])), results[key]['throughput'], marker=markers[i], color=colors[i], label=key)
        if len(results[key][xkey]) > len(longest): longest = results[key][xkey]
    ax0.set_xticks(np.arange(len(longest)))
    ax0.set_xticklabels(longest)
    bot, up = ax0.get_ylim()
    ax0.set_ylim(0, up*2)
    ax0.legend()

    ax1.set_xlabel(xlabel)
    ax1.set_ylabel('median latency')
    for i, key in enumerate(results):
        ax1.plot(np.arange(len(results[key][xkey])), results[key]['50_latency'], marker=markers[i], color=colors[i], label=key)
    ax1.set_xticks(np.arange(len(longest)))
    ax1.set_xticklabels(longest)
    bot, up = ax1.get_ylim()
    ax1.set_ylim(0, up)

    ax2.set_xlabel(xlabel)
    ax2.set_ylabel('consensus latency')
    for i, key in enumerate(results):
        if '50_totalorder_latency' in results[key] and len(results[key]['50_totalorder_latency']) > 0:
            ax2.plot(np.arange(len(results[key][xkey])), results[key]['50_totalorder_latency'], marker=markers[i], color=colors[i], label=key)
    ax2.set_xticks(np.arange(len(longest)))
    ax2.set_xticklabels(longest)
    bot, up = ax2.get_ylim()
    ax2.set_ylim(0, up)

    plt.savefig(folder + '/' + prefix + '_scalability.png')

def new_plot(folder, results, prefix, metric = 'latency'):
    fig, (ax0, ax1, ax2) = plt.subplots(nrows = 3)
    
    ax0.set_xlabel('throughput')
    ax0.set_ylabel('median_' + metric)
    for i, key in enumerate(results):
        ax0.plot(results[key]['throughput'], results[key]['50_' + metric], marker=markers[i], color=colors[i], label=key)
    ax0.legend()

    ax1.set_xlabel('throughput')
    ax1.set_ylabel('90%_' + metric)
    for i, key in enumerate(results):
        ax1.plot(results[key]['throughput'], results[key]['90_' + metric], marker=markers[i], color=colors[i], label=key)
 
    ax2.set_xlabel('throughput')
    ax2.set_ylabel('99%_' + metric)
    for i, key in enumerate(results):
        ax2.plot(results[key]['throughput'], results[key]['99_' + metric], marker=markers[i], color=colors[i], label=key)
 
    plt.savefig(folder + '/' + prefix + '_throughput_%s.png' % metric)

def analysis(folder, high_limit, low_limit, prefix, filter):
    res = {'num_client_threads':[], 'num_replicas':[], 'throughput':[], 'avg_latency':[], 'avg_totalorder_latency':[], '50_latency':[], '90_latency':[],
           '99_latency':[], '50_totalorder_latency':[], '90_totalorder_latency':[], '99_totalorder_latency':[]}
    files = [f for f in os.listdir(folder) if f.find(filter) >= 0]
    if prefix.startswith('server_inc'):
        func = lambda s: int(s.split('_')[3])
    else:
        func = lambda s: int(s.split('_')[5].split('.')[0])
    files.sort(key=func)
    for f in files:
        items = func(f)
        if high_limit and items > high_limit: continue
        if low_limit and items < low_limit: continue

        fp = open(folder + '/' + f)
        print (folder + '/' + f)
        v = json.load(fp)
        for k in res:
            if k in v:
                res[k].append(v[k])
            elif k in v['exp_config']:
                res[k].append(v['exp_config'][k])
        fp.close()
    return res

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print ('Usage: python plot.py <input folder> <exp> <output folder> <beta>')
        exit(1)

    high_limit = None
    low_limit = None
    results = {}
    beta = sys.argv[4].split(',')
    for b in beta:
        results['Archipelago-C(beta=%s)' % b] =  analysis(sys.argv[1] + '/archipelago/' + sys.argv[2], high_limit, low_limit, sys.argv[2], '_beta_%s_' % b)
        if len(results['Archipelago-C(beta=%s)' % b]['num_replicas']) == 0: del results['Archipelago-C(beta=%s)' % b]
        results['Concord(beta=%s)' % b] = analysis(sys.argv[1] + '/concord/' + sys.argv[2], high_limit, low_limit, sys.argv[2], '_beta_%s_' % b)
        results['Concord(beta=%s)' % b]['50_latency'] = results['Concord(beta=%s)' % b]['50_totalorder_latency']
        results['Concord(beta=%s)' % b]['90_latency'] = results['Concord(beta=%s)' % b]['90_totalorder_latency']
        results['Concord(beta=%s)' % b]['99_latency'] = results['Concord(beta=%s)' % b]['99_totalorder_latency']
        if len(results['Concord(beta=%s)' % b]['num_replicas']) == 0: del results['Concord(beta=%s)' % b]
    for key in results:
        print ("%s:%r" % (key, results[key]))
    plot(sys.argv[3], results, sys.argv[2])
    new_plot(sys.argv[3], results, sys.argv[2], 'latency')
    new_plot(sys.argv[3], results, sys.argv[2], 'totalorder_latency')
    print ('finish')
