#! /bin/bash

node_data_dir="/data/amigo/node1"
exe_full_path="/data/amigo/node1/bin/witness_node"

nodepid=`ps -ef|grep witness_node|grep $node_data_dir|grep -v "grep"|awk '{print $2}'`


echo "nodepid: $nodepid"
if [ "$nodepid" == "" ]; then
  echo "no witness node found for $node_data_dir"
  exit 1
else
  echo "to kill witness node. pid: $nodepid"
  kill $nodepid
fi

node=`ps -ef|grep witness_node|grep $node_data_dir`
if [ "$node"x=x ]; then
  echo "witness node1 stop successfully"
else
  echo "witness node1 stop failed"
fi
