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

cd ${project.build.directory}/node-classes

echo "preparing the node environment incl. required dependencies"

#currently, we assume npm and node is installed on the machine running this script
#download and install all required node modules using the package.json
#npm install

#so far, only files located at the ${project.build.outputDirectory} can find the installed node_modules
#make the installed node_modules available for the test-classes
ln -sf ${project.build.directory}/node-classes/node_modules ..

cd ${project.build.directory}/test-classes
ln -sf ${project.build.directory}/test-classes/spec ..

echo "running the tests"

#currently, we assume npm and node is installed on the machine running this script
#${project.build.directory}/nodejs/node ${project.build.directory}/node_modules/jasmine-node/lib/jasmine-node/cli.js --requireJsSetup ${project.build.testOutputDirectory}/node_require.config.js --matchall $@ $TESTS
cd ${project.build.directory}
node ${project.build.directory}/test-classes/node-run-unit-tests.js

exit
