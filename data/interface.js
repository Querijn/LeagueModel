var skinsByChampion = {};
var wasUrlSkinRequested = false;

var consoleLog = console.log;
var consoleWarn = console.warn;
var consoleError = console.error;

var log = [];
function logMessage(type, message) {
	log.push(`[${(new Date()).toLocaleString()}] '${type.toUpperCase()}: ${message}'`);
}

console.log = function () { 
	var text = arguments[0];
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join(" ");
	
	logMessage("log", text);
	consoleLog.apply(console, arguments);
}

console.warn = function () { 
	var text = arguments[0];
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join(" ");

	logMessage("warn", text);
	consoleWarn.apply(console, arguments);
}

console.error = function () { 
	var text = arguments[0];
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join(" ");

	logMessage("error", text);
	consoleError.apply(console, arguments);
}

function UploadLog() {

	if (!confirm("This will send information from your browser and your recent log to https://irule.at.\n\nThings that this sends:\n- Your error log, everything caught during run-time that was reported with console.log, console.warn and console.error.\n- The navigator javascript object, to identify your device (https://developer.mozilla.org/en-US/docs/Web/API/Navigator)\n- The screen object, to identify your window state (https://developer.mozilla.org/en-US/docs/Web/API/Screen)\n\nIf you are not okay with this, you can click cancel below. Otherwise this information is stored for 3 months."))
		return;
	
	var reason = prompt("What went wrong?");
	
	var nav = {}, view = {};
	for (var i in navigator) nav[i] = navigator[i];
	for (var i in screen) view[i] = screen[i];
	var blob = new Blob([JSON.stringify({
		reason: reason,
		log: log,
		screen: view,
		navigator: nav
	})]);
	
	var reader = new FileReader();
	reader.onload = function(event){
		var formData = new FormData();
		formData.append('fname', 'log.txt');
		formData.append('data', event.target.result);
		
		var logUploader = new XMLHttpRequest();
		logUploader.onreadystatechange = function() {
			if (this.readyState == 4 && this.status == 200)
				console.log(this.responseText);
		};
		
		logUploader.open("POST", "https://irule.at/models/log_upload/", true);
		logUploader.send(formData);
	};
	reader.readAsDataURL(blob);
}

fetch("data/small-summary.json")
.then(function(response) {
	if (response.status !== 200) {
		console.error(response.error);
		return;
	}

	response.json().then(async data => {
		var champSelect = document.getElementById("champ-select");
		var skinSelect = document.getElementById("skin-select");

		let added = false;
		for (let champ of data) {
			var option = document.createElement("option");
			option.text = champ.name;
			option.value = champ.id.toLowerCase();
			champSelect.add(option);

			skinsByChampion[option.value] = champ.skins;

			if (added) continue;
			for (let skin of champ.skins) {
				var option = document.createElement("option");
				option.text = skin.name;
				option.value = skin.num;
				skinSelect.add(option);
			}

			added = true;
		}
	});
})
.catch(function(err) {
	console.error("Fetch error:", err);
});

document.getElementById("champ-select").addEventListener("change", () => {
	var champSelect = document.getElementById("champ-select");
	var skinSelect = document.getElementById("skin-select");
	var champ = champSelect.options[champSelect.selectedIndex].value.toLowerCase();
	var skinSet = skinsByChampion[champ];

	for (i = skinSelect.options.length - 1; i >= 0; i--) {
		skinSelect.remove(i);
	}

	for (let skin of skinSet) {
		var option = document.createElement("option");
		option.text = skin.name;
		option.value = skin.num;
		skinSelect.add(option);
	}
});

function LoadSkin() {
	var champSelect = document.getElementById("champ-select");
	var skinSelect = document.getElementById("skin-select");
	var champ = champSelect.options[champSelect.selectedIndex].value.toLowerCase();
	var skin = skinSelect.options[skinSelect.selectedIndex].value;
	
	fetch(`data/output/data/characters/${champ}/animations/skin${skin}.bin`, { method: 'HEAD' })
	.then(response => {
		
		var animationSkin = (response.status == 200) ? skin : 0;

		Module.LoadSkin(
			`data/output/data/characters/${champ}/skins/skin${skin}.bin`,
			`data/output/data/characters/${champ}/animations/skin${animationSkin}.bin`
		);
	})
	.catch(error => {
	    console.error(`Error occurred trying to load animations/skin${skin}.bin: ${error}`);
		Module.LoadSkin(
			`data/output/data/characters/${champ}/skins/skin${skin}.bin`,
			`data/output/data/characters/${champ}/animations/skin0.bin`
		);
	});
	
	
}

function SetupAnimationHTML() {
	var animationCount = Module.GetAvailableAnimations();
	console.log("We have " + animationCount + " animations.");

	var animationSelect = document.getElementById("animation-select");
	for (i = animationSelect.options.length - 1; i >= 0; i--) {
		animationSelect.remove(i);
	}

	for (var i = 0; i < animationCount; i++) {
		var animName = Module.GetAnimationName(i);
		var shortName = animName.substr(animName.lastIndexOf('/') + 1);

		var option = document.createElement("option");
		option.text = shortName;
		option.value = animName;
		animationSelect.add(option);
	}
}

function PlayAnimation() {
	var animationSelect = document.getElementById("animation-select");
	var skin = Module.GetSkinName(0);
	var animation = animationSelect.options[animationSelect.selectedIndex].value;
	Module.PlayAnimation(skin.toLowerCase(), animation.toLowerCase());
}

var Module = {
	preRun: [],
	postRun: [],
	
	print: function(text) {
		if (arguments.length > 1)
			text = Array.prototype.slice.call(arguments).join(" ");

		console.log(text);
	},

	printErr: function(text) {
		if (arguments.length > 1)
			text = Array.prototype.slice.call(arguments).join(" ");
	
		console.error(text);
	},

	OnReady: function() {

		SetupAnimationHTML();
		if (wasUrlSkinRequested) return;
		wasUrlSkinRequested = true;
		
		var url = new URL(window.location.href);
		var champKey = url.searchParams.get("champKey");
		if (!champKey) return;

		var skinId = url.searchParams.get("skinId") || 0;
		console.log(champKey, skinId);
		Module.LoadSkin(
			`data/output/data/characters/${champKey}/skins/skin${skinId}.bin`,
			`data/output/data/characters/${champKey}/animations/skin${skinId}.bin`
		);
	},

	canvas: (function() {
		var canvas = document.getElementById("canvas");

		canvas.addEventListener(
			"webglcontextlost",
			function(e) {
				Module.printErr("WebGL context lost. You will need to reload the page.");
				e.preventDefault();
			},
			false
		);
		return canvas;
	})()
};