#!/bin/bash

sudo apt install -y cgroup-tools libssl-dev

cat <<EOF > cgconfig.conf
mount {
  memory = /var/cgroups;
}
group lert {
  perm {
    task {
      uid = `id -u`;
      gid = `id -g`;
      fperm = 770;
    }
    admin {
      uid = `id -u`;
      gid = `id -g`;
      fperm = 770;
    }
  }
  memory {
  }
}
EOF

sudo umount /var/cgroups
sudo cgconfigparser -l cgconfig.conf
#echo $MEM | sudo tee /var/cgroups/mantis/memory.limit_in_bytes
#echo 3 | sudo tee /proc/sys/vm/drop_caches
#/usr/bin/time cgexec -g memory:mantis $*
