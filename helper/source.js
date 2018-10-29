const processArguments = process.argv.filter((v, i) => i >= 2);
const instruction = processArguments[0];

const fs = require("fs-extra");
const path = require("path");

const CMakeFilePath = "CMakeLists.txt";
const BackupCMakeFilePath = "BackupCMakeLists.txt";

const headerTemplate = "helper/template/header.hpp";
const sourceTemplate = "helper/template/source.cpp";

const cmakeFile = fs.readFileSync(CMakeFilePath);
if (!cmakeFile) throw new Error(`${CMakeFilePath} could not be opened!`);
let cmake = cmakeFile.toString();
const projectNameRegex = /project\((.*?)\)/g.exec(cmake);
if (!projectNameRegex || projectNameRegex.length < 1) throw new Error(`Could not find the project name in ${CMakeFilePath}.`);

const projectName = projectNameRegex[1];
const addSourceFunctionName = `ADD_${projectName.toUpperCase()}_SOURCE(`;

async function main() {

    switch (instruction) {
        case "add": {
            await addClass(processArguments[1], processArguments[2]);
            break;
        }

        case "format": {
            await format();
            break;
        }

        default: {
            console.error(`Unknown instruction '${instruction}'.`);
        }
    }
}

function getCStyle(variable) {

    let result = "";
    let ignoreUnderscore = false;

    for (let i = 0; i < variable.length; i++) {

        let character = variable.charAt(i);
        if (/[A-Z]/.test(character)) {

            if (!ignoreUnderscore) {
                if (i != 0) result += "_";
                ignoreUnderscore = true;
            }

            result += character.toLowerCase();
            continue;
        }
        else if (/[A-Za-z]/.test(character) == false) {
            ignoreUnderscore = true;
            result += character;
            continue;
        }

        ignoreUnderscore = false;
        result += character;
    }

    return result;
}

function getAddSourceCodeLines() {

    let lines = [];
    let firstIndex = -1;
    let lastIndex = -1;

    for (let i = -1; i < cmakeFile.length;) {

        i = cmake.indexOf(addSourceFunctionName, i + 1);
        if (firstIndex < 0) firstIndex = i;
        if (i == -1) break;

        let nextOpen = i + addSourceFunctionName.length;
        let nextClose = i + addSourceFunctionName.length;
        do {
            nextOpen = cmake.indexOf('(', nextClose);
            nextClose = cmake.indexOf(')', nextClose);
        }
        while (nextClose > nextOpen);

        const line = cmake.substr(i, nextClose - i + 1);
        lines.push({ index: i, line });
    }

    return lines;
}

function parseLineParts(line) {

    const lineParts = [];

    let lastIndex = 0;
    let inString = false;
    for (let i = 0; i < line.length; i++) {

        const char = line.charAt(i);

        switch (char) {
            default:
                break;

            case '"':
            case '\'': {
                if (!inString) {
                    inString = true;
                    break;
                }
                else {
                    inString = false;
                    break;
                }
            }

            case '\t':
            case ' ':
                if (inString) break;
                lineParts.push(line.substr(lastIndex, i - lastIndex + 1).trim());
                lastIndex = i;
                break;
        }
    }

    lineParts.push(line.substr(lastIndex).trim());
    return lineParts.filter(l => l.length > 0);
}

function setSourceCodeLines(lines) {

    const start = Math.min.apply(Math, lines.map(function (line) { return line.index > 0 ? line.index : 9e9; }));
    const end = Math.max.apply(Math, lines.map(function (line) { return line.index > 0 ? line.index + line.line.length + 1 : -9e9; }));

    lines.sort((a, b) => {
        a = a.line.toUpperCase();
        b = b.line.toUpperCase();
        return (a < b) ? -1 : (a > b) ? 1 : 0;
    })

    const linePartsLines = [];
    for (const line of lines) {
        linePartsLines.push(parseLineParts(line.line));
    }

    // Determine sizes
    const size = [0, 0, 0];
    for (let i = 0; i < 3; i++) {
        for (const line of linePartsLines) {
            if (line[i].length > size[i]) {
                size[i] = line[i].length;
            }
        }
    }

    // Apply tabs
    for (let i = 0; i < 2; i++) {
        for (const line of linePartsLines) {
            let lengthDiff = (size[i] - line[i].length) + 4;
            for (let j = 0; j < lengthDiff; j++) {
                line[i] += ' ';
            }
        }
    }

    const newLines = [];
    for (const line of linePartsLines) {
        newLines.push(line.join(""));
    }

    cmake = cmake.substr(0, start) + newLines.join("\n") + "\n" + cmake.substr(end);

    fs.writeFileSync(CMakeFilePath, cmake);
}

function addSourceLineToCMake(ideLocation, fileName) {

    const lines = getAddSourceCodeLines();
    lines.push({ index: -1, line: `${addSourceFunctionName}${ideLocation} ${fileName}.hpp ${fileName}.cpp)` });
    setSourceCodeLines(lines);
}

async function addSourceFiles(className, destination) {

    const headerLocation = destination + ".hpp";
    const sourceLocation = destination + ".cpp";
    const includeFolder = headerLocation.substr("source/".length);

    console.log("Copying source files..");
    const headerCopy = fs.copy(headerTemplate, headerLocation);
    const sourceCopy = fs.copy(sourceTemplate, sourceLocation);

    await Promise.all([headerCopy, sourceCopy]);
    console.log("Done copying source files..");

    console.log("Replacing source file variables..");

    const headerFile = fs.readFileSync(headerLocation);
    if (!headerFile) throw new Error(`${headerLocation} could not be opened!`);
    let headerContents = headerFile.toString();

    const sourceFile = fs.readFileSync(sourceLocation);
    if (!sourceFile) throw new Error(`${sourceLocation} could not be opened!`);
    let sourceContents = sourceFile.toString();

    var files = [
        {
            contents: sourceContents,
            location: sourceLocation,
            promise: null
        },
        {
            contents: headerContents,
            location: headerLocation,
            promise: null
        }
    ];

    files.forEach((file) => {

        const actualClassName = className.substr(className.lastIndexOf('/') + 1);
        file.contents = file.contents.replace(/\$\$CLASS_NAME\$\$/g, actualClassName);
        file.contents = file.contents.replace(/\$\$HEADER_LOCATION\$\$/g, includeFolder);

        file.promise = fs.writeFile(file.location, file.contents);
    });

    await Promise.all(files.map(f => f.promise));
    console.log("Done replacing source file variables..");
}

function addClass(name, fileLocation) {

    if (name.toLowerCase().indexOf("module") >= 0) {
        fileLocation = `source/modules/${getCStyle(name)}`;

        addSourceLineToCMake(`Modules\\\\${name}`, fileLocation);
    }
    else {
        let cmakeLocation = fileLocation.replace(/\//g, '\\\\');
        fileLocation = path.join(`source`, fileLocation, getCStyle(name)).replace(/\\/g, '/');


        // Make all words start with caps
        let chars = [ "\\\\", "_" ];
        for (let i = 0; i < chars.length; i++) {
            let cmakeDirectories = cmakeLocation.split(chars[i]);
            cmakeDirectories = cmakeDirectories.map(w => w.charAt(0).toUpperCase() + w.slice(1));
            cmakeLocation = cmakeDirectories.join(chars[i] == "_" ? "" : chars[i]);
        }

        addSourceLineToCMake(cmakeLocation, fileLocation);
    }

    addSourceFiles(name, fileLocation);
}

function format(name) {
    setSourceCodeLines(getAddSourceCodeLines());
}

main();