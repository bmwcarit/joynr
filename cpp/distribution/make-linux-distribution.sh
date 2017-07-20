###
# #%L
# joynr::C++
# %%
# Copyright (C) 2011 - 2017 BMW Car IT GmbH
# %%
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# #L%
###
#!/bin/sh

################################################################################
# Makes a Linux distribution as a tar file
################################################################################

[ $# -eq 1 ] || { cat << EOUSAGE ; exit 1 ; }

  Usage: $0 version

Create a tar file containing a joynr C++ distribution.
Arguments:
  version - The version of joynr

EOUSAGE

# Get the version
version=$1

# Get the location of the joynr source tree
current_directory=`pwd`
script_location=`dirname $0`
cd "${script_location}/.."
joynr_cpp_root=`pwd`
cd "${current_directory}"

# Build the filename of the tar file
os=`uname -o | tr / -`
processor=`uname -p`
glibc=`ldd --version | head -1 | awk '{print $NF}'`
name="JOYnr_${version}_${os}_${processor}_glibc-${glibc}"

# Create the directory structure of the distribution
mkdir "${name}"
cp -r joynr.cmake "${joynr_cpp_root}/bin" "${name}"
mkdir "${name}/libs"

# Create the tar file
tar czf "${current_directory}/${name}.tar.gz" "${name}"

# Clean up the directory structure
rm -rf "${name}"

