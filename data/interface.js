var skinsByChampion = {};
var wasUrlSkinRequested = false;

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
				if (champ.id < 0) continue;
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
		console.error("Fetch Error :-S", err);
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
	Module.LoadSkin(
		`data/output/data/characters/${champ}/skins/skin${skin}.bin`,
		`data/output/data/characters/${champ}/animations/skin${skin}.bin`
	);
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

var log = [];
var statusElement = document.getElementById("status");
var progressElement = document.getElementById("progress");
var spinnerElement = document.getElementById("spinner");
var Module = {
	preRun: [],
	postRun: [],
	
	print: (function() {
		var element = document.getElementById("output");
		if (element) element.value = ""; // clear browser cache
		return function(text) {
			if (arguments.length > 1)
				text = Array.prototype.slice.call(arguments).join(" ");
			// These replacements are necessary if you render to raw HTML
			//text = text.replace(/&/g, "&amp;");
			//text = text.replace(/</g, "&lt;");
			//text = text.replace(/>/g, "&gt;");
			//text = text.replace('\n', '<br>', 'g');
			console.log(text);
			if (element) {
				element.value += text + "\n";
				element.scrollTop = element.scrollHeight; // focus on bottom
			}
		};
	})(),

	printErr: function(text) {
		if (arguments.length > 1)
			text = Array.prototype.slice.call(arguments).join(" ");
		if (0) {
			// XXX disabled for safety typeof dump == 'function') {
			dump(text + "\n"); // fast, straight to the real console
		} else {
			console.error(text);
		}
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