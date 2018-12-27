var skinsByChampion = {};

fetch("data/small-summary.json")
.then(function(response) {
    if (response.status !== 200) {
        console.error(response.error)
        return;
    }

    response.json().then(async(data) => {
        var champSelect = document.getElementById("champ-select");
        var skinSelect = document.getElementById("skin-select");

        let added = false;
        for (let champ of data)
        {
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

    for(i = skinSelect.options.length - 1 ; i >= 0 ; i--) {
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
    Module.LoadSkin(`data/output/data/characters/${champ}/skins/skin${skin}.bin`, `data/output/data/characters/${champ}/animations/skin${skin}.bin`);
}