#!/bin/bash
docker run --rm -it --name quisp -v "$(pwd):/root/quisp" -u "$(id -u):$(id -g)" quisp /bin/sh <<-EOF
    ls -la
    pwd
    cd /root
    ls -la 
    cd /root/quisp/test/
    /bin/bash test.sh
EOF
# docker exec -it quisp /bin/bash
