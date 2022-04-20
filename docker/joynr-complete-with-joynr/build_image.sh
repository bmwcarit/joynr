#!/bin/bash
set -x

dockerRepository=""
dockerImage=joynr-base
dockerTempImageName=joynr-complete-with-joynr-tmp
qualifiedDockerImageName=${dockerRepository}${dockerImage}
resultingDockerImage=joynr-complete-with-joynr
scriptPath=$(cd $(dirname $0) && pwd)
repoPath=$(cd $(dirname $0)/../.. && pwd)
docker create -t -v ${scriptPath}:/data/src-helper -v ${repoPath}:/data/src --privileged --name ${dockerTempImageName} ${qualifiedDockerImageName}

# 'docker exec' the script to get root access rights
docker start ${dockerTempImageName}
docker exec -t --privileged ${dockerTempImageName} /data/src-helper/cpp-install-joynr.sh 
docker stop ${dockerTempImageName}

docker commit ${dockerTempImageName} ${resultingDockerImage}
docker rm -f ${dockerTempImageName}
