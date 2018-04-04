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

# fail on first error
# exit immediately if a command exits with a non-zero status
# print commands before they are executed
set -e -x

# format all js files in
# - src/main/js and its subdirectories
# - subdirectories of src/test/js but not in src/test/js/integration
# - src/test/js
node_modules/.bin/prettier \
    --write \
    --config scripts/prettier.config.json \
    "src/{{main/js/!(lib),test/js/!(integration)}/**/*.js,test/js/*.js}"

