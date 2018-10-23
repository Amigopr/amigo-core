#! /bin/bash

#nohup /data/amigo/console/bin/witness_node --replay-blockchain  --max-ops-per-account 3000 --partial-operations true  -d /data/amigo/node1/witness_node_data_dir 2>&1 >> /data/amigo/node1/witness_node_data_dir/logs/console.log &
nohup /data/amigo/console/bin/witness_node --max-ops-per-account 3000 --partial-operations true -d /data/amigo/node1/witness_node_data_dir 2>&1 >> /data/amigo/node1/witness_node_data_dir/logs/console.log &

