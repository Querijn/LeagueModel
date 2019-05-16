# LeagueModel
An application to display League of Legends models on Windows and Web. You can find it at https://irule.at/models

## How do I use it?

Simply download one of the releases, found [here](https://github.com/Querijn/LeagueModel/releases). Put it on a webserver and copy the `data` folder in the root to it. Then, finally, extract the champion data you wish from their wad.client files in your copy of League of Legends. 

If you have no way of extracting the models, you can run a command-line from the `extractor` folder and run `LeagueExtract.exe -source="C:/Riot Games/League of Legends/RADS/solutions/lol_game_client_sln/releases/<current version>/deploy/DATA/FINAL/Champions/"`. You require to replace `<current_version>` with the latest version, and you might need tot adjust the whole path altogether. 

If you don't know what folder to look for, download [Everything](https://www.voidtools.com/support/everything/), install and run it, and look for `Aatrox.wad.client`. The folder that it is in, is the path you need to put up in there. If you cannot find it still, please install League of Legends.

By default this command outputs to `../data/output/`, which is where it needs to go relative from the `extractor` folder, but if you need to change it to something else, add `-output="<your output path>"`.

## How do I compile it?

For PC:
- Get CMake, and run the GUI. The Visual Studio 2017 generators work, both for x86 and x64.
- Run it, generate it, and open the project. 
- Set startup application to `LeagueModel`.
- Compile and run it.

For web:
- Install Emscripten. Setup the environment either globally or in the current command-line.
- Install node.js if it didn't come with Emscripten for some reason.
- Go to the root of this project (where the `data`, `src` and `include` folders are)
- run `node helper/build prod`.
- Your build should, if it built correctly, appear in `build/web_prod`. 
- Put your data folder in there, or make a symbolic link to the one in the root.
- Alternatively, you can append `copy-data` to the previous command to do this after a successful build, but this is not advisable since it has to copy a lot of data in most use-cases.

### Special thanks

- Honux from Teemo.GG, for inspiring me to put even more work in this.
- The CDragon people (Stelar7, Le Poussin), for helping me out decyphering these files.
- Literally [this video](https://www.youtube.com/watch?v=F-kcaonjHf8) for fixing my animations. Thanks, ThinMatrix.
- Everyone that had a model viewer for me to compare my output against.
- [Radu Pașparugă](https://radupasparuga.github.io/) for the HTML/CSS redesign!
