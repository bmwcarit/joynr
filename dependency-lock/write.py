#!/usr/bin/python3
# -*- coding: utf-8 -*-
# #%L
# %%
# Copyright (C) 2021 BMW Car IT GmbH
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

'''
Writes the generated.pom based on the source.pom.
All prior manual modifications are reverted.
'''
import argparse
import logging
import os
import sys

sys.path.append(os.path.abspath(os.path.dirname(__file__)))
import update

'''Entry point'''
if __name__ == '__main__':
    update.init(argparse.ArgumentParser(description="Write dependency lock"))
    generator = update.PomGenerator()
    try:
        generator.reset()
        projectDependencies = update.ProjectDependency()
        externalDependencies = projectDependencies.listExternal()
        generator.write(externalDependencies)
        # The first approach contains multiple versions of maven plugin dependencies.
        # Rerun to consolidate.
        externalDependencies = projectDependencies.listExternal()
        generator.write(externalDependencies)
        logging.getLogger("main").info(
            "Write dependency lock. %d external dependencies found." % len(externalDependencies))
    except Exception:
        generator.restore()
        raise
