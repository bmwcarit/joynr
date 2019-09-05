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
import glob = require("glob");
import * as util from "util";
import * as handlebars from "handlebars";
import * as fs from "fs";
import mkdirp = require("mkdirp");
import _ = require("lodash");

interface FidlFilesJson {
    interfaces: Record<string, string[]>;
}

const globAsync = util.promisify(glob);

const version = require("../package.json").version;
const generatorName = `joynr-generator-standalone-${version}.jar`;
const generatorPath = path.join(__dirname, "../jars", generatorName);
const fidlFileDesc = `Json file containing .fidl files grouped by fidlFileGroups used to create joynr-includes.
Automatically sets -i option.
interface FidlFilesJson {
    interfaces: Record<string (fidlFileGroup), string[] (relative Paths to fidl files)>;
}`;

// eslint-disable-next-line no-console
const log = console.log;
// eslint-disable-next-line no-console
const error = console.error;

async function main(): Promise<void> {
    const argv = yargs
        .usage("Usage: $0 [options]")
        .example(`$0 -m Radio.fidl -m Test.fidl -o src-gen`, "compile 2 different .fidl files")
        .example(`$0 -m model -js`, "use all fidl files inside the ’model’ directory. Compile to js")
        .example(
            `$0 -f model fidl-files.json`,
            `compile all .fidl files listed in fidl-files.json and generate includes`
        )
        .option("modelPath", {
            alias: "m",
            desc: "path to a directory with fidl files, or a single fidl file (can be supplied multiple times)"
        })
        .option("fidlFile", {
            alias: "f",
            desc: fidlFileDesc
        })
        .option("outputPath", { alias: "o", demandOption: true, desc: "output path will be created if not existing" })
        .option("js", {
            alias: "j",
            boolean: true,
            desc: "compile to js with d.ts instead of ts"
        })
        .option("includes", {
            alias: "i",
            boolean: true,
            desc: "create joynr includes"
        })
        .help()
        .wrap(yargs.terminalWidth()).argv;

    const outputPath = argv.outputPath as string;
    const modelPath = argv.modelPath as string | undefined;
    const fidlFile = argv.fidlFile as string | undefined;

    if (!modelPath && !fidlFile) {
        error("Please provide either modelPath or fidlFile option");
        process.exit(1);
    }

    let parsedJson: FidlFilesJson;

    let modelPathArray: string[] = Array.isArray(modelPath) ? modelPath : modelPath ? [modelPath] : [];
    const baseArray = modelPathArray;

    if (fidlFile) {
        parsedJson = JSON.parse(fs.readFileSync(fidlFile, "utf8"));
        Object.values(parsedJson.interfaces).forEach(paths => {
            modelPathArray = modelPathArray.concat(paths);
        });
    }

    await generateTSSources(modelPathArray, outputPath);
    if (fidlFile) {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        Object.entries(parsedJson!.interfaces).forEach(([fidlFileGroup, fidlFiles]) => {
            createJoynrIncludes(fidlFiles, outputPath, fidlFileGroup);
        });
    } else if (argv.includes) {
        let files: string[] = [];
        baseArray.forEach(modelPath => {
            if (modelPath.endsWith(".fidl")) {
                files.push(modelPath);
            } else {
                // assume directory
                files = files.concat(glob.sync(`${modelPath}/**/*.fidl`));
            }
        });
        createJoynrIncludes(files, outputPath);
    }
    const files = (await globAsync(`${outputPath}/**/*.ts`)).filter(file => !file.includes(".d.ts"));
    if (argv.js) {
        compileToJS(files);
    }
    log("All done!");
    process.exit(0);
}

async function generateTSSources(modelPaths: string | string[], outputPath: string): Promise<void> {
    ([] as string[]).concat(modelPaths).forEach(path => {
        const command =
            `java -jar ${generatorPath}` +
            ` -modelPath ${path} -outputPath ${outputPath}` +
            ` -generationLanguage javascript`;
        log(`executing command: ${command}`);
        execSync(command);
    });
    log(`ts generation done`);
}

/**
 * Compile JS files to TS according to
 * https://github.com/microsoft/TypeScript/wiki/Using-the-Compiler-API
 * @param fileNames list of files to be compiled
 */
function compileToJS(fileNames: string[]): void {
    log(`compiling ${fileNames.length} files to JS`);
    const compileOptions = {
        noEmitOnError: true,
        noImplicitAny: true,
        strictNullChecks: true,
        noUnusedParameters: true,
        noImplicitThis: true,
        target: ts.ScriptTarget.ES2017,
        module: ts.ModuleKind.CommonJS,
        esModuleInterop: true,
        declaration: true
    };

    const program = ts.createProgram(fileNames, compileOptions);
    const emitResult = program.emit();

    const allDiagnostics = ts.getPreEmitDiagnostics(program).concat(emitResult.diagnostics);

    allDiagnostics.forEach(diagnostic => {
        if (diagnostic.file) {
            // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
            const { line, character } = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start!);
            const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n");
            log(`${diagnostic.file.fileName} (${line + 1},${character + 1}): ${message}`);
        } else {
            log(`${ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n")}`);
        }
    });

    const exitCode = emitResult.emitSkipped ? 1 : 0;
    log(`Process exiting with code '${exitCode}'.`);
    process.exit(exitCode);
}

// Setup the template file that will be used to generate the JavaScript file(s). A handler
// must be registered so Handlebars can determine if a variable is an object or not.
handlebars.registerHelper("ifObject", function(this: any, item, options) {
    if (typeof item === "object") {
        return options.fn(this);
    } else {
        return options.inverse(this);
    }
});

handlebars.registerHelper("concat", (...args) => {
    return new handlebars.SafeString(
        args
            .slice(0, -1)
            .filter(Boolean)
            .join("_")
    );
});

/**
 * Scans a directory, creating a map of data in order to generate the JavaScript file.
 *
 * @returns An object mapping bracket notation interface to file path to required file
 * @param dir The directory to begin scanning for files
 * @param relativeFromDir The file path will be calculated as a relative path
 * from this directory
 */
function createRequiresFromDir(dir: string, relativeFromDir: string): Record<string, any> {
    const files = fs.readdirSync(dir);
    const modulePaths: Record<string, any> = {};

    files.forEach(file => {
        const fullPath = path.join(dir, file);
        const currentFileStat = fs.statSync(fullPath);

        if (currentFileStat.isDirectory()) {
            const subDirModules = createRequiresFromDir(fullPath, relativeFromDir);
            const transformedSubDirModules: Record<string, any> = {};
            Object.keys(subDirModules).forEach(subModule => {
                transformedSubDirModules[subModule] = subDirModules[subModule];
            });
            modulePaths[file] = transformedSubDirModules;
        } else if (currentFileStat.isFile()) {
            // potential issue if basename equals other directory name.
            const moduleName = path.basename(file);
            // remove Suffix to get only one entry for .js .ts and .d.ts in case old compiled code is still there
            const moduleNameWithoutSuffix = moduleName.split(".")[0];
            const modulePath = path.relative(relativeFromDir, path.join(dir, moduleNameWithoutSuffix));
            modulePaths[moduleNameWithoutSuffix] = modulePath;
        }
    });

    return modulePaths;
}

function createJoynrIncludes(
    fidlFiles: string[],
    outputFolder: string,
    fidlFileGroup: string = "joynr-includes"
): void {
    log(`creating joynr includes for fidlFiles ${JSON.stringify(fidlFiles)} and fidlFileGroup ${fidlFileGroup}`);
    const templateFilePath = path.join(__dirname, "joynr-require-interface.hbs");
    const templateFile = fs.readFileSync(templateFilePath, "utf8");
    const requiresTemplate = handlebars.compile(templateFile, { noEscape: true });

    const requiresPerFile: Record<string, any> = {};

    fidlFiles.forEach(fidlFile => {
        // Gather the package name in order to construct a path, generate the method
        // names and paths and then generate the file's contents.
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const fidlFileContents = fs.readFileSync(fidlFile, "utf8")!;
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const packagePath = fidlFileContents.match(/^package\s+(.+)$/m)![1];
        const packagePathParts = packagePath.split(".");
        // eslint-disable-next-line prefer-spread
        const outputPathSuffix = path.join.apply(path, packagePathParts);
        const outputFolderPerGroup = path.join(outputFolder, fidlFileGroup);
        const newFilename = path.join(outputFolderPerGroup, `${packagePathParts.pop()}`);

        const requires = createRequiresFromDir(
            path.join(outputFolder, "joynr", outputPathSuffix),
            outputFolderPerGroup
        );

        requiresPerFile[newFilename] = _.merge(requiresPerFile[newFilename], requires);

        try {
            mkdirp.sync(outputFolderPerGroup);
        } catch (e) {
            if (e.code !== "EEXIST") {
                throw e;
            }
        }
    });

    for (const file in requiresPerFile) {
        if (requiresPerFile.hasOwnProperty(file)) {
            const requires = requiresPerFile[file];
            fs.writeFileSync(`${file}.ts`, requiresTemplate({ requires, fileName: path.basename(file, ".ts") }));
        }
    }
}

if (!module.parent) {
    main().catch(err => {
        log(err);
    });
}
