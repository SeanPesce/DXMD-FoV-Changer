# Enhanced FoV Options for Deus Ex: Mankind Divided  
**Author: Sean Pesce**  

<p align="center">
<img align="center" title="Example FoV presets" src="https://seanpesce.github.io/Portfolio/assets/img/showcase/dxmd-fov/01.gif" alt="Example FoV presets (GIF of several in-game screenshots)" width="450px">
<br><br>
<img align="center" title="Separate FoV for first-person hands" src="https://seanpesce.github.io/Portfolio/assets/img/showcase/dxmd-fov/02.gif" alt="Demo of hands FoV (GIF of several in-game screenshots)" width="450px">
</p>  


## Overview  
This mod enhances Field of View settings for [Deus Ex: Mankind Divided](http://store.steampowered.com/app/337000) and [Deus Ex: Breach](http://store.steampowered.com/app/555450). Players can set a default FoV in the configuration file and/or [change the FoV](https://www.youtube.com/watch?v=qWjd7HO216A) on the fly with configurable key-binds. Most notably, this mod enables higher & lower FoV values than those supported by the vanilla game, and lets the user [change the rendered FoV of the first-person hands separately](https://seanpesce.github.io/Portfolio/assets/img/showcase/dxmd-fov/02.gif). Without a separate hands FoV, weapon animations can look awful on higher FoV settings (see the [showcase GIF](https://seanpesce.github.io/Portfolio/assets/img/showcase/dxmd-fov/02.gif) above).  

## Installation  
Copy **`bink2w64.dll`** and **`DXMD_FOV.ini`** into the `\retail\` directory (usually `C:\Program Files (x86)\Steam\SteamApps\common\Deus Ex Mankind Divided\retail`). This will overwrite the game's original `bink2w64.dll`, but the mod automatically drops the original DLL back into the directory at startup and loads it in the DLL chain, so game functionality is preserved.  

**Legacy versions:** Older releases of the mod shipped as **`version.dll`** instead. For those mod versions, copy `version.dll` into the same `\retail\` directory. This method no longer works on Windows 11 because `version.dll` is no longer loaded from the game directory.


## Default key-binds  
Increase FoV: +  
Decrease FoV: -  
Increase Hands FoV: *Disabled*  
Decrease Hands FoV: *Disabled*  
Restore Player Preferred FoV: Backspace  
Restore Game default FoV: Delete  
  
You can change the key-binds and default FoV values by navigating to your `\Deus Ex Mankind Divided\retail\` folder and editing `DXMD_FOV.ini`.  

## **[Download the latest version here!](https://github.com/SeanPesce/DXMD-FoV-Changer/releases)**  


## Contact  
If you find any bugs, please contact me on [GitHub](https://github.com/SeanPesce/DXMD-FoV-Changer/issues/new), [Reddit](https://www.reddit.com/u/SeanPesce), or my [personal website](https://SeanPesce.github.io).  


## Built With  
 * [Visual Studio](https://www.visualstudio.com/)  
 * [Stephan Brumme's Portable C++ Hashing Library](https://github.com/stbrumme/hash-library)  


## Acknowledgements  
 * Reddit users **[lovelymelons4sale](https://www.reddit.com/user/lovelymelons4sale)**, **[Donaldduck8](https://www.reddit.com/user/Donaldduck8)**, and **[Dollynho250](https://www.reddit.com/user/Dollynho250)** - Helped in determining the source of an elusive crash on Windows 7  


## License  
[GNU General Public License v3.0](LICENSE)  


---------------------------------------------

For unrelated inquiries and/or information about me, visit my **[personal website](https://SeanPesce.github.io)**.  

