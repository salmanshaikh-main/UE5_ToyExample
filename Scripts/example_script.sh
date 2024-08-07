#!/bin/bash

player_id=$1
delay_duration=2  # 4 seconds delay
effect_duration=10  # 10 seconds total duration

# Add delay rule
sudo tc qdisc add dev lo root netem delay ${delay_duration}s

# Sleep for the effect duration
sleep $effect_duration

#Remove delay rule
sudo tc qdisc del dev lo root
