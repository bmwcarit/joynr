###
# #%L
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
set -e

cd ${project.build.directory}/test-classes/node/shutdown

echo "preparing the environment for the node shutdown test"
npm run-script preinstall

echo "running the node shutdown test"

NODE_VERSION_MAJOR=$(node --version | cut -d v -f 2 | cut -d . -f 1)
if [ $NODE_VERSION_MAJOR -ge 6 ]
then
    # --trace-warnings was added in node v6.0.0
    node --trace-warnings --trace-deprecation NodeShutdownTest.js
else
    node --trace-deprecation NodeShutdownTest.js
fi

exit
