#!/usr/bin/python3
# -*- coding: utf-8 -*-
# #%L
# %%
# Copyright (C) 2022 BMW Car IT GmbH
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
Remove unused dependencies from the generated.pom
Add missing dependencies to the generated.pom.
Note that this process does not update existing dependency versions.
A manual check/update is required if new direct dependencies are
added to the projects or their versions are updated.
'''

import argparse
import collections
import filecmp
import logging
import os
import shutil
import subprocess
import xml.etree.ElementTree as xmlET

DEPENDENCY_LOCK_DIR = os.path.abspath(os.path.dirname(__file__))

# Scope not relevant for dependency management.
Dependency = collections.namedtuple(
    'Dependency', ['groupId', 'artifactId', 'type', 'version'])


class PomGenerator(object):
    DEFAULT_WORKING_DIR = os.path.dirname(__file__)

    def __init__(self, lockDir=os.path.abspath(DEFAULT_WORKING_DIR)):
        object.__init__(self)
        self.__log = logging.getLogger('PomGenerator')
        self.__pomFile = os.path.join(lockDir, 'pom.xml')
        self.__backupFile = os.path.join(lockDir, "pom-backup.%s" % id(self))
        self.__pomHeader = []
        self.__pomFooter = []
        try:
            if not os.path.isfile(self.__pomFile):
                raise RuntimeError(
                    "POM file '%s' does not exist." % self.__pomFile)

            pomSection = self.__pomHeader
            with open(self.__pomFile) as fp:
                for line in fp:
                    if line.strip() == '</dependencies>':
                        pomSection = self.__pomFooter
                    pomSection.append(line)
                    if line.strip() == '<dependencies>':
                        pomSection = []  # Drop dependencies

            if 0 == len(self.__pomFooter):
                raise RuntimeError(
                    "POM file '%s' does not contain 'dependencies' section." % self.__pomFile)

            shutil.copyfile(self.__pomFile, self.__backupFile)
        except Exception as e:
            self.__log.error(
                "PomGenerator initialization arguments incorrect: %s" % e)
            raise

    def __del__(self):
        if os.path.isfile(self.__backupFile):
            os.remove(self.__backupFile)

    def restore(self):
        shutil.copyfile(self.__backupFile, self.__pomFile)

    def reset(self):
        self.write([])

    def modified(self):
        return not filecmp.cmp(self.__backupFile, self.__pomFile, shallow=False)

    def write(self, dependencies):
        with open(self.__pomFile, 'w', encoding='utf-8') as fp:
            for line in self.__pomHeader:
                fp.write(line)
            for dependency in dependencies:
                fp.write(self.__toXml(dependency))
            for line in self.__pomFooter:
                fp.write(line)

    def __toXml(self, dependency):
        nodeName = 'dependency'
        node = xmlET.Element(nodeName)
        for name, value in dependency._asdict().items():
            child = xmlET.SubElement(node, name)
            child.text = value

        nodeStr = xmlET.tostring(node, encoding='unicode', method='xml')
        nodeStrTags = nodeStr.split('<',)
        nodeStr = ''
        for nodeStrTag in nodeStrTags:
            if nodeName in nodeStrTag:
                nodeStr += "\t\t\t<%s\n" % nodeStrTag
            elif nodeStrTag:
                if nodeStrTag.startswith('/'):
                    nodeStr += "<%s\n" % nodeStrTag
                else:
                    nodeStr += "\t\t\t\t<%s" % nodeStrTag
        return nodeStr


class ProjectDependency(object):
    DEFAULT_POM_DIR = os.path.abspath(os.path.join(
        os.path.dirname(__file__), os.path.normpath('../')))
    DEFAULT_POM_FILENAME = 'pom.xml'
    MVN_EXEC = 'mvn'
    MVN_SCOPES = ['compile', 'provided', 'runtime', 'test', 'system', 'import']
    ADDITIONAL_PROFILES = []

    def __init__(self, pomDir=DEFAULT_POM_DIR, pomFile=DEFAULT_POM_FILENAME):
        object.__init__(self)
        pomDir = os.path.abspath(pomDir)
        self.__log = logging.getLogger('ProjectDependency')
        try:
            if not os.path.isdir(pomDir):
                raise RuntimeError(
                    "POM directory '%s' does not exist." % pomDir)

            pomFile = os.path.join(pomDir, pomFile)
            if not os.path.isfile(pomFile):
                raise RuntimeError("POM file '%s' does not exist." % pomFile)
            mvnCall = subprocess.run(
                [ProjectDependency.MVN_EXEC, '-version'], capture_output=True, text=True)
            if mvnCall.returncode:
                raise RuntimeError("'%s' execution failed:\n%s\n%s" % (
                    ProjectDependency.MVN_EXEC, mvnCall.stdout, mvnCall.stderr))

            self.__cmd = [ProjectDependency.MVN_EXEC, '-f', pomFile]
            if ProjectDependency.ADDITIONAL_PROFILES:
                self.__cmd.append('-P')
                self.__cmd.append(','.join(ProjectDependency.ADDITIONAL_PROFILES))
        except Exception as e:
            self.__log.error(
                "ProjectDependency initialization arguments incorrect: %s" % e)
            raise

    def listTransitives(self):
        allDependencies = self.__set()
        self.__log.debug("Found %d dependencies." % len(allDependencies))

        directDependencies = self.__set(['excludeTransitive'])
        self.__log.debug("Found %d direct dependencies." %
                         len(directDependencies))

        transitiveDependencies = allDependencies - directDependencies
        return sorted(transitiveDependencies)

    def listExternal(self, internalGroupsStartWith='io.joynr'):
        allDependencies = self.__set()
        self.__log.debug("Found %d dependencies." % len(allDependencies))

        externalDependencies = sorted(filter(
            lambda d: not d.groupId.startswith(internalGroupsStartWith), allDependencies))
        self.__log.debug("Found %d external dependencies." %
                         len(externalDependencies))
        return externalDependencies

    def __set(self, userProperties=[]):
        cmd = self.__cmd.copy()
        for userProperty in userProperties:
            cmd += ["-D%s" % userProperty]
        cmd += [
            '-pl',
            '!io.joynr.android:slf4j-android-bindings,'
            '!io.joynr.android:libjoynr-android-websocket-runtime,'
            '!io.joynr.android:joynr-android-binder-runtime,'
            '!io.joynr.examples:android-hello-world,'
            '!io.joynr.examples:android-hello-world-consumer,'
            '!io.joynr.examples:android-hello-world-provider,'
            '!io.joynr.examples:android-hello-world-binder,'
            '!io.joynr.examples:android-hello-world-consumer-binder,'
            '!io.joynr.examples:android-hello-world-provider-binder',
            '-am',
            'dependency:list',
            '-Dno-android-sdk'
        ]
        mvnCall = subprocess.run(cmd, capture_output=True, text=True)
        if mvnCall.returncode:
            raise RuntimeError("'%s' execution failed:\n%s\n%s" % (
                " ".join(cmd), mvnCall.stdout, mvnCall.stderr))

        return self.__parseOutput(mvnCall.stdout)

    def __parseOutput(self, output):
        dependencies = set()
        for line in output.split("\n"):
            if not line:
                continue
            line = line.strip()
            endOfInfo = line.find(' ')
            line = line[endOfInfo:]
            line = line.strip()
            parts = line.split(':')
            if 5 == len(parts) and self.__startsWithValidScope(parts[4]):
                del parts[-1]
                dependencies.add(Dependency(*parts))
        return dependencies

    def __startsWithValidScope(self, scopePart):
        for supportedScopes in ProjectDependency.MVN_SCOPES:
            if scopePart.startswith(supportedScopes):
                return True
        return False

def init(parser):
    parser.add_argument('--loglevel', '-l', dest='loglevel', required=False,
                        default=logging.INFO, type=int, help="Log level, default is %d (info)" % logging.INFO)
    args = parser.parse_args()
    logging.basicConfig(level=args.loglevel)


'''Entry point'''
if __name__ == '__main__':
    init(argparse.ArgumentParser(description="Update dependency lock"))
    generator = PomGenerator()
    try:
        projectDependencies = ProjectDependency()
        externalDependencies = projectDependencies.listExternal()
        generator.write(externalDependencies)
        logging.getLogger("main").info(
            "Updated dependency lock. %d external dependencies found." % len(externalDependencies))
    except Exception:
        generator.restore()
        raise
