# Enhanced FoV Options for Deus Ex: Mankind Divided  
Author: Sean Pesce

**[Demo Video](https://www.youtube.com/watch?v=qWjd7HO216A)** (or [Webm](https://gfycat.com/FavorableThoseEagle))



This mod allows you to change the Field of View in DXMD (including higher or lower values than the game normally allows) at the press of a button. It also allows you to change the rendered FoV of your hands separately, so weapon animations won't look strange on higher FoV settings.

Installation: Install the [x64 Visual C++ 2015 Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=48145), and then merge this \Deus Ex Mankind Divided\ folder with the one in your Steam library directory (usually `C:\Program Files (x86)\Steam\SteamApps\common\Deus Ex Mankind Divided\`)

**NOTE:** If you already had Special K installed before installing this, make sure you dxgi.ini has the following in it:  
[Import.DXMD_FOV]  
Architecture=x64  
Filename=DXMD_FOV.dll  
When=Lazy  
Role=ThirdParty  

Default keybinds:  
Increase FoV: +  
Decrease FoV: -  
Increase Hands FoV: period  
Decrease Hands FoV: comma  
Restore Player Preferred FoV: Backspace  
Restore Game default FoV: Delete  

You can change the keybinds and default FoVs by going to \Deus Ex Mankind Divided\retail\ and editing DXMD_FOV.ini


## OTHER FEATURES  
I was messing around with some other features that I saw requested on the Deus Ex subreddit (https://www.reddit.com/r/DeusEx ), and added them to this mod.

**Achievement Monitoring:**  
I added an option for the mod to check whether the currently-loaded savegame is still eligible for the Pacifist achievement at the press of a button. You can set the hotkey in the DXMD_FOV.ini file (checkPacifistStatus_Keybind). 1 beep means you're still eligible for the achievement; 3 beeps means you've killed someone and are no longer eligible.  
Additionally, you can have the mod automatically monitor your eligibility for this achievement on the fly, by setting monitorPacifist=1 in DXMD_FOV.ini. If your character becomes ineligible, the mod will alert you with 3 beeps.  
Default Keybind:  
Check Pacifist achievement eligibility: Numpad Minus (-)


**Challenges:**  
You can set ToggleRegen_Keybind to a key, which allows you to turn off health/energy regen for challenge runs.  
Default Keybind:  
Toggle Regen: Numpad Plus (+)

**HUD Mods:**  
Unfortunately these kept getting broken after every patch, so all HUD mods have been disabled (probably permanently, unless I find time to work on this mod again in the future)


**Credit:**  
This mod is built on top of Kaldaien's Special K dll, which also adds support for other stuff (such as texture dumping). To edit Special K settings, go to the \Deus Ex Mankind Divided\ folder and edit dxgi.ini. You can find more info about Special K here: https://github.com/Kaldaien/SpecialK/releases/tag/sk_043



If you find any bugs, please contact me at:  
https://www.reddit.com/u/SeanPesce
