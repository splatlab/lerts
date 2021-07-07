#!/bin/bash

# Setup the cgroup profile and memory constraints.
# FOR SETTING UP CGROUPS THE USER NEEDS SUDO ACCESS.
sudo apt-get install cgroup-bin cgroup-lite cgroup-tools cgroupfs-mount libcgroup1
sudo cp cgconfig.conf /etc/
sudo cgconfigparser -l /etc/cgconfig.conf

