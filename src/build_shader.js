const fs = require("fs");
const child_process = require("child_process");
const path = require("path");

const cwd = path.resolve(process.cwd());
const toolLocation = path.resolve("ext/sokol/tools/bin/win32/sokol-shdc.exe");
if (fs.existsSync(toolLocation) == false) {
    console.error(`Shader compiler '${toolLocation}' does not exist!`);
    return;
}

const folders = ["src/shaders", "src/shaders/generated" ];
(async function () {
    for (let folder of folders) {
        if (folder.startsWith("#") || folder == "")
            continue;

        if (fs.existsSync(folder) == false) {
            console.warn(`Shader folder '${folder}' does not exist!`)
            continue;
        }

        let files = fs.readdirSync(folder).filter(file => file.endsWith(".glsl")).filter(file => fs.lstatSync(path.join(folder, file)).isFile()).map(file => file);
        try {
            emptyDir(path.join(folder, "gen"));
            let promises = [];
            for (let file of files) {
                const fullFile = path.join(folder, file);
                const hasProgramDirective = fs.readFileSync(fullFile).toString().split("\n").findIndex(l => l.startsWith("@program"));
                if (hasProgramDirective < 0) {
                    console.log(`- Skipping shader for "${file}" (no program)`);
                    continue;
                }

                const name = file.split(".")[0];
                const output = `${name}.hpp`;
                const genOutput = path.join(folder, "gen", output);
                const finalOutput = path.join(folder, output);
                const command = `"${toolLocation}" --input ${fullFile} --output ${genOutput} --slang glsl330:glsl300es:hlsl5:glsl100 --ifdef`;
    
                // Run command
                promises.push(execAsync(command, cwd).then(() => {
                    if (copyIfDifferent(finalOutput, genOutput) == false)
                        console.log(`- Generated shader for "${file}" (no changes)`);
                }));
            }
    
            await Promise.all(promises);
            emptyDir(path.join(folder, "gen"));
        }
        catch (e) {
            console.error(e);
        }
    }

    console.log("Done.");
})();


function emptyDir(directory) {
	if (!fs.existsSync(directory)) {
		fs.mkdirSync(directory);
		return;
	}

	const files = fs.readdirSync(directory);
	for (const file of files)
		fs.unlinkSync(path.join(directory, file));
}


function copyIfDifferent(path, generatedPath) {
	if (fs.existsSync(generatedPath) == false) {
		return true;
	}

    const oldExists = fs.existsSync(path);
    const oldContent = oldExists ? fs.readFileSync(path).toString() : "_INVALID@@@@@";
	const newContent = fs.readFileSync(generatedPath).toString();
	if (oldContent == newContent)
		return false;

	fs.writeFileSync(path, newContent);
	console.log(`- Written to "${path}".`);
    return true;
}

async function execAsync(cmd, cwd) {
	return new Promise((resolve, reject) => {
		// console.log("Executing: " + cmd);
		child_process.exec(cmd, { timeout: 400000, cwd }, (error, stdout, stderr) => {
			if (error || stderr.length > 0) {
				reject((error.stack || error.message || "") + "\nSTDERR: " + stderr + "\nSTDOUT: " + stdout);
				return;
			}
			resolve({ error, stdout, stderr });
		});
	});
}