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

import { log } from "./logging";
import { prettyLog } from "./logging";

function showHelp(
    modes: Record<string, { value: string; description: string; options: Record<string, string> }>
): void {
    let optionsText;
    let modeKey;
    let optionKey;
    prettyLog("The following options are available: ");
    for (modeKey in modes) {
        if (modes.hasOwnProperty(modeKey)) {
            const mode = modes[modeKey];
            optionsText = "";
            for (optionKey in mode.options) {
                if (mode.options.hasOwnProperty(optionKey)) {
                    optionsText += `${mode.options[optionKey]}/`;
                }
            }
            if (optionsText !== "") {
                optionsText = optionsText.substr(0, optionsText.length - 1);
                optionsText = `[${optionsText}]`;
            }
            log(`${mode.value} ${optionsText}    ${mode.description}`);
        }
    }
    log("");
}

export = showHelp;
