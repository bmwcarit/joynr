#! /usr/bin/env node
/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

import yargs = require("yargs");
import path = require("path");
import { execSync } from "child_process";
import * as ts from "typescript";
import * as glob from "glob";
import * as util from "util";

const globAsync = util.promisify(glob);

const version = require("../package.json").version;
const generatorName = `joynr-generator-standalone-${version}.jar`;
const generatorPath = path.join(__dirname, "../jars", generatorName);

async function main() {
    const argv = yargs
        .usage("Usage: $0 [options]")
        .example(`$0 -m Radio.fidl -m Test.fidl -o src-gen`, "compile 2 different .fidl files")
        .option("modelPath", {
            alias: "m",
            desc: "path to a directory with fidl files, or a single fidl file (can be supplied multiple times)"
        })
        .option("outputPath", { alias: "o", demandOption: true, desc: "output path will be created if not exist" })
        .option("js", {
            boolean: true,
            desc: "compile to js instead of ts"
        })
        .help()
        .wrap(yargs.terminalWidth()).argv;
    await generateTSSources(argv.modelPath as string | string[], argv.outputPath as string);
    const files = await globAsync(`${argv.outputPath}/**/*.ts`);
    compileToJS(files);
}

async function generateTSSources(modelPaths: string | string[], outputPath: string) {
    ([] as string[]).concat(modelPaths).forEach(path => {
        const command =
            `java -jar ${generatorPath}` +
            ` -modelPath ${path} -outputPath ${outputPath}` +
            ` -generationLanguage javascript`;
        console.log(`executing command: ${command}`);
        execSync(command);
    });
    console.log(`ts generation done`);
}

/**
 * Compile JS files to TS according to
 * https://github.com/microsoft/TypeScript/wiki/Using-the-Compiler-API
 * @param fileNames list of files to be compiled
 */
function compileToJS(fileNames: string[]) {
    const compileOptions = {
        noEmitOnError: true,
        noImplicitAny: true,
        strictNullChecks: true,
        noUnusedParameters: true,
        noImplicitThis: true,
        target: ts.ScriptTarget.ES2017,
        module: ts.ModuleKind.CommonJS,
        esModuleInterop: true
    };

    const program = ts.createProgram(fileNames, compileOptions);
    const emitResult = program.emit();

    const allDiagnostics = ts.getPreEmitDiagnostics(program).concat(emitResult.diagnostics);

    allDiagnostics.forEach(diagnostic => {
        if (diagnostic.file) {
            const { line, character } = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start!);
            const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n");
            console.log(`${diagnostic.file.fileName} (${line + 1},${character + 1}): ${message}`);
        } else {
            console.log(`${ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n")}`);
        }
    });

    const exitCode = emitResult.emitSkipped ? 1 : 0;
    console.log(`Process exiting with code '${exitCode}'.`);
    process.exit(exitCode);
}

if (!module.parent) {
    main().catch(err => {
        console.log(err);
    });
}
