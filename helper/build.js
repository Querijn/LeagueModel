const fs = require("fs-extra");
const path = require("path");
const { exec } = require('child_process');
const util = require('util');

const compiler = "emcc";
const includeFolders = [ "src/", "include/" ];
const sourceFolders = [ "src/" ];
const buildInfoFileName = "helper/build.json";
const baseBuildFolder = "build/web_";

const procArgs = process.argv.filter((v, i) => i >= 2);
const isProduction = procArgs[0] && procArgs[0].toLowerCase().startsWith("prod");
const isWasm = !procArgs.includes("no-wasm");
const buildFolder = baseBuildFolder + (isProduction ? "prod" : "dev") + (!isWasm ? "_js/" : "/");
const foldersToCopy = [ "data/" ];
console.log(`Running a ${isProduction ? "production" : "develop"} build ${isWasm ? "as WebAssembly" : "without WebAssembly"}.`);

const devShell = "helper/dev_index.html";
const prodShell = "helper/prod_index.html";
const shellFile = isProduction ? prodShell : devShell;

const requiredArgs = [ "-std=c++11", "-s", "FULL_ES2=1", "-Werror", "-s", "ASSERTIONS=2", "-s", "DEMANGLE_SUPPORT=1",  "-s", "ALLOW_MEMORY_GROWTH=1", "-s", "SAFE_HEAP=1" ]; // , "--shell-file", shellFile ];
const devArgs = [ "-g4", "--source-map-base", "http://localhost:8080/", ];
const prodArgs = [ "-Os", "--closure", "1", "-Walmost-asm" ];

function fileBackedObject(path) {
    const contents = fs.readFileSync(path, "utf8");
    const obj = JSON.parse(contents);

    return generateProxy(obj, path);
}

function generateProxy(obj, path) {
    const proxy = {
        set(object, property, value, receiver) {
            Reflect.set(object, property, value, receiver);
            fs.writeFileSync(path, JSON.stringify(obj));
            return true;
        },

        get(object, property, receiver) {
            const child = Reflect.get(object, property, receiver);
            if (!child || typeof child !== "object") return child;
            return new Proxy(child, proxy);
        },
    };

    return new Proxy(obj, proxy);
}

function FindAllFiles(dir, filter) {
    let results = [];

    if (!filter) {
        console.error("No filter function setup");
        return;
    }

    if (!fs.existsSync(dir)) {
        console.error(`"${dir}" is not an existing directory.`);
        return;
    }

    var files = fs.readdirSync(dir);
    for (var i = 0; i < files.length; i++) {
        var filename = path.join(dir, files[i]);
        var stat = fs.lstatSync(filename);

        if (stat.isDirectory())
            results = results.concat(FindAllFiles(filename, filter));
        else if (filter(filename, stat))
            results.push(filename.replace(/\\/g, "/"));
    }

    return results;
}

function run(args) {

    const buildCommand = args.join(" ");
    return new Promise((resolve, reject) => {
        // console.log(`Running command "${buildCommand}"`);
        exec(buildCommand, (err, stdout, stderr) => {

            if (err) {
                reject(err);
                return;
            }

            if (stderr && stderr.length != 0) {
                reject(stderr);
                return;
            }

            resolve(stdout);
        });
    });
}

function getModifyTime(file) {
    let stat;
    try {
        stat = fs.statSync(file);
    }
    catch (e) {
        return new Date(0);
    }
    return stat.mtime;
}

function shouldBuild(sourceFile, buildFileTime) {
    const latestChange = getModifyTime(sourceFile);
    if (latestChange > buildFileTime)
        return true;
    else {
        let cppCode;
        try {
            cppCode = fs.readFileSync(sourceFile).toString();
        } 
        catch (e) {
            return false; // This means it's most likely an external piece of code
        }

        const regex = /^\s?#\s?include(\s?)[<"](.*?)[>"]$/gm;

        var match;

        while (true) {
            match = regex.exec(cppCode);
            if (!match) break;

            for (let include of includeFolders)
                if (shouldBuild(include + match[2], buildFileTime))
                    return true;
        }

        return false;
    }
}

fs.ensureDirSync(buildFolder);
fs.ensureFileSync(buildInfoFileName);
let buildInfoFile = fileBackedObject(buildInfoFileName);

const finalArgs = requiredArgs.concat(isProduction ? prodArgs : devArgs); // Copy
if (!isWasm) finalArgs.push("-s", "WASM=0");

// Add includes
for (let include of includeFolders)
    finalArgs.push("-I" + include);

(async () => {
    let sourceFiles = [];

    // Add source files
    for (let sourceFolder of sourceFolders)
        sourceFiles = sourceFiles.concat(FindAllFiles(sourceFolder, (a_Filename, a_Stat) => a_Filename.endsWith(".cpp") && a_Filename.indexOf("glm") < 0 && a_Filename.indexOf("windows") < 0));

    const buildFiles = [];

    let buildTime;
    if (isProduction)
    {
        if (isWasm) buildTime = buildInfoFile.productionBuildTime;
        else if (!isWasm) buildTime = buildInfoFile.productionBuildTimeJS;
    }
    else
    {
        if (isWasm) buildTime = buildInfoFile.developmentBuildTime;
        else if (!isWasm) buildTime = buildInfoFile.developmentBuildTimeJS;
    }

    let hasChanges = false;
    console.log(`Building ${sourceFiles.length} source files.`);
    for (let i = 0; i < sourceFiles.length; i++) {

        let sourceFile = sourceFiles[i];

        let buildFile = `./${buildFolder}${sourceFile}.bc`;
        buildFiles.push(buildFile);

        if (fs.existsSync(buildFile) && !shouldBuild(sourceFile, new Date(buildTime || 0))) {
            console.log(`[${i + 1}/${sourceFiles.length}] Skipping "${sourceFile}" (no changes)`);
            continue;
        }

        fs.ensureFileSync(buildFile);

        try {
            let arg = [compiler, sourceFile, "-o", buildFile].concat(finalArgs);
            //console.log(`[${i + 1}/${sourceFiles.length}] Building "${sourceFile} (${finalArgs.join(" ")})"`);
            console.log(`[${i + 1}/${sourceFiles.length}] Building "${sourceFile}"`);
            await run(arg);
            hasChanges = true;

            if (isProduction)
            {
                if (isWasm) buildTime = buildInfoFile.productionBuildTime = new Date().getTime();
                else if (!isWasm) buildTime = buildInfoFile.productionBuildTimeJS = new Date().getTime();
            }
            else
            {
                if (isWasm) buildTime = buildInfoFile.developmentBuildTime = new Date().getTime();
                else if (!isWasm) buildTime = buildInfoFile.developmentBuildTimeJS = new Date().getTime();
            }

            buildInfoFile.buildCount = sourceFiles.length;
        }
        catch (e) {
            console.error(e);
            return;
        }
    }

    if (!hasChanges && sourceFiles.length == buildInfoFile.buildCount) {
        console.log("No changes detected, skipping final combine.");
        return;
    }

    console.log("Combining build files");
    finalCommand = [compiler].
        concat(buildFiles.length ? buildFiles : sourceFiles).
        concat(["-o", `./${buildFolder}index.html`]).
        concat(finalArgs);

    try {
        await run(finalCommand);
    }
    catch (e) {
        console.error(e);
    }

    for (let folder of foldersToCopy) {
        console.log(`Copying folder "${folder}" to "${buildFolder}"`);
        fs.copySync(folder, buildFolder + folder);
    }
})();