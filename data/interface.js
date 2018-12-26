fetch("data/champion-summary.json")
.then(function(response) {
    if (response.status !== 200) {
        console.error(response.error)
        return;
    }

    response.json().then(async(data) => {
        var select = document.getElementById("champ-select");

        console.log(data[1]);
        for (let champ of data)
        {
            if (champ.id < 0) continue;
            var option = document.createElement("option");
            option.text = champ.name;
            option.value = champ.alias;
            select.add(option);
        }
    });
})
.catch(function(err) {
    console.error("Fetch Error :-S", err);
});

function LoadSkin() {
    var select = document.getElementById("champ-select");
    var champ = select.options[select.selectedIndex ].value.toLowerCase();
    Module.LoadSkin(`data/output/data/characters/${champ}/skins/skin0.bin`, `data/output/data/characters/${champ}/animations/skin0.bin`);
}