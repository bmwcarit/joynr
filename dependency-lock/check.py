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
Check that the dependency lock file is up to date.
Returns zero if check passes.
Assures that lock file is restored after update.
'''
import argparse
import logging
import os
import sys

sys.path.append(os.path.abspath(os.path.dirname(__file__)))
import update

'''Entry point'''
if __name__ == '__main__':
    update.init(argparse.ArgumentParser(description="Check dependency lock"))
    generator = update.PomGenerator()
    generatedPomChanged = False
    try:
        projectDependencies = update.ProjectDependency()
        externalDependencies = projectDependencies.listExternal()
        generator.write(externalDependencies)
        generatedPomChanged = generator.modified()
        if generatedPomChanged:
            logging.getLogger("main").warning(
                "Dependency lock file update required.")
        else:
            logging.getLogger("main").info("Dependency lock file up to date.")

    finally:
        generator.restore()

    sys.exit(1 if generatedPomChanged else 0)
