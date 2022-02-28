#!/bin/sh

# Access shared volumes as non root user
#
# Usage: docker run -e DEV_UID=$(id -u) ...
# Source: http://chapeau.freevariable.com/2014/08/docker-uid.html

USER=joynr

export DEV_UID=${DEV_UID:=1000}

useradd -u $DEV_UID -m $USER
chown -R $USER:$USER /home/$USER
# override required for using jdk11 with maven
# otherwise jdk8 will still be taken despite
# alternatives has already been configured for java11
# since the set_jvm function in /usr/share/java-utils/java-functions
# invoked by shell script /usr/bin/mvn does not respect alternatives
cat > /home/$USER/.mavenrc << EOF
export JAVA_HOME=/usr/lib/jvm/java-openjdk
EOF

#add user to kvm group and change /dev/kvm to joynr user so that it can 
#run android virtual devices inside the container
usermod -a -G kvm joynr
chown joynr:joynr /dev/kvm

cd /home/$USER

if [ $# -eq 0 ]; then
        # no commands supplied
        export HOME=/home/$USER
        exec gosu $DEV_UID /bin/bash
else
        # commands supplied
        export HOME=/home/$USER
        exec gosu $USER /bin/bash -c "$*"
fi
