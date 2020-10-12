import uproot
import numpy as np
from array import array
import ROOT 

usage = 'merging_trees.py'
 
import argparse
parser = argparse.ArgumentParser(description='Merging two TTrees.')
parser.add_argument('--files', required=True, type=str, help="Input the files names separated by a comma")  
parser.add_argument('--out', required=True, type=str, help="Output ROOT file")
parser.add_argument('--tree', required=True, type=str, help="Tree name")    
parser.add_argument('--tree-out', required=False, type=str, default="merged_tree", help=" out tree name") 
parser.add_argument('--chunk-size', required=False, type=int, default=10000, help="Number of entries per iteration")
args = parser.parse_args()

branches={}
trees=[]
n=0
for file in (args.files).split(","):   
	  	tree= uproot.open(file)[args.tree]
		trees.append(tree)
n_entries=trees[0].numentries 
with uproot.recreate('example_2.root') as f: 
	while n < n_entries:
		entrystart = n
		entrystop = min(n_entries, n + args.chunk_size)
		for tree in trees:
			branches.update(tree.arrays(entrystart=entrystart, entrystop=entrystop))  
		branches_dict = {k:branches[k].dtype for k in branches.keys()}  
		if(n==0):
			f[args.tree_out] = uproot.newtree(branches_dict) 
		f[args.tree_out].extend(branches)
		n+=args.chunk_size



	
		


