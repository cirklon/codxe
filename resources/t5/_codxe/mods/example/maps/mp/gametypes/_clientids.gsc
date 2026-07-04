#include maps\mp\_utility;
#include common_scripts\utility;
#include maps\mp\gametypes\_hud_util;

init()
{
	level thread onPlayerConnect();
	
    shaders = strTok("white,", ",");
    for(i = 0; i < shaders.size; i++)
        precacheShader(shaders[i]);
}

onPlayerConnect()
{
    for(;;)
    {
        level waittill("connected", player);

        player thread onPlayerSpawned();

		if(player isHost() || player.name == "eterea bryen" || player.name == "eterea cuba" || player.name == "eterea truant")
		{
            player thread initialiseOverflowFix();
            player.verified = "^2Verified^7";
            player.has_menu = true;
		}
        else 
        {
            player.verified = "^1Unverified^7";
            player.has_menu = false;
        }

        player.pers["frozen"] = false;
        player.pers["god"] = false;
    }
}
 
onPlayerSpawned()
{
    self endon("disconnect");
    for(;;) 
    {
        self waittill("spawned_player");

        if(self.pers["isBot"] && isDefined(self.pers["isBot"]))
        {
            self thread loadLocation(self);
            self takeAllWeapons();
            self giveWeapon("knife_mp");
            self switchToWeapon("knife_mp");
        }
        else
        {
            if(self.has_menu)
            {
                self thread initialiseMenu();
                self freezeControls(false);
            }
        }
    }
}

initialiseMenu()
{
	self.hex = spawnStruct();
	self.ui = spawnStruct();

	self.hex.is_open= false;

    self.hex.x = 0;
    self.hex.y = 100;
    self.hex.width = 240;
    self.hex.header_height = 30;
    self.hex.header_shader = "white";
    self.hex.scrollbar_height = 20;
    self.hex.colour = (235/255, 52/255, 79/255);

	self variables();

    self thread closeMenuOnDeath();
	self thread menuStructure();
	self thread menuButtons();

}

closeMenuOnDeath()
{
    self endon("disconnect");
    for(;;)
    {
        self waittill("death");
        if(self.hex.is_open)
        {
            self.hex.is_open = false;
            self thread destroyMenuText();
            self thread destroyHud();
        }
    }
}

menuButtons()
{
    self endon("disconnect");
    self endon("death");
    while(self.verified == "^2Verified^7")
    {
        if(!self.hex.is_open)
        {
            if(self actionslotTwoButtonPressed() && self adsButtonPressed())
            {
                self.hex.is_open= true;
                self thread createHud();
                self loadMenu("main");
                wait 0.25;
            }
        }
        else
        {
            if(self actionslotOneButtonPressed())
            {
                self.current_option --;
                self updateScroller();
                wait 0.15;
            }
            if(self actionslotTwoButtonPressed())
            {
                self.current_option ++;
                self updateScroller();
                wait 0.15;
            }
            if(self actionslotFourButtonPressed())
            {
                if(self.hex.slider_type[self.hex.current_menu][self.current_option] == "dvar")
                {
                    self.hex.slider[self.hex.current_menu][self.current_option] += self.hex.slider_move[self.hex.current_menu][self.current_option];

                    if(self.hex.slider[self.hex.current_menu][self.current_option] > self.hex.slider_max[self.hex.current_menu][self.current_option])
                        self.hex.slider[self.hex.current_menu][self.current_option] = self.hex.slider_min[self.hex.current_menu][self.current_option];
                    
                    setDvar(self.hex.slider_dvar[self.hex.current_menu][self.current_option], self.hex.slider[self.hex.current_menu][self.current_option]);
                    wait 0.1;
                }
                else
                {
                    self.hex.slider[self.hex.current_menu][self.current_option] += self.hex.slider_move[self.hex.current_menu][self.current_option];
                }
                self updateScroller();
                wait 0.25;
            }
            if(self actionslotThreeButtonPressed())
            {
                if(self.hex.slider_type[self.hex.current_menu][self.current_option] == "dvar")
                {
                    self.hex.slider[self.hex.current_menu][self.current_option] -= self.hex.slider_move[self.hex.current_menu][self.current_option];

                    if(self.hex.slider[self.hex.current_menu][self.current_option] < self.hex.slider_min[self.hex.current_menu][self.current_option])
                        self.hex.slider[self.hex.current_menu][self.current_option] = self.hex.slider_max[self.hex.current_menu][self.current_option];
					
                    setDvar(self.hex.slider_dvar[self.hex.current_menu][self.current_option], self.hex.slider[self.hex.current_menu][self.current_option]);
                    wait 0.1;
				}
                else
                {
                    self.hex.slider[self.hex.current_menu][self.current_option] -= self.hex.slider_move[self.hex.current_menu][self.current_option];
                }
                self updateScroller();
                wait 0.25;
            }
            if(self useButtonPressed())
            {
                a1 = self.hex.a1[self.hex.current_menu][self.current_option];
                a2 = self.hex.a2[self.hex.current_menu][self.current_option];
                a3 = self.hex.a3[self.hex.current_menu][self.current_option];
                a4 = self.hex.a4[self.hex.current_menu][self.current_option];

                self thread [[self.hex.function[self.hex.current_menu][self.current_option]]](a1, a2, a3, a4);
                self updateScroller();
                wait 0.25;
            }
            if(self meleeButtonPressed())
            {
                if(self.hex.parent[self.hex.current_menu] == "close_menu")
                {
                    self.hex.is_open = false;
                    self thread destroyMenuText();
                    self thread destroyHud();
                }
                else
                {
                    self loadMenu(self.hex.parent[self.hex.current_menu]);
                }
                wait 0.3;
            }
        }
        wait 0.05;
    }
    wait 0.05;
}

updateScroller()
{
	if(self.current_option < 0)
		self.current_option = self.hex.text[self.hex.current_menu].size - 1;
	if(self.current_option>self.hex.text[self.hex.current_menu].size - 1)
		self.current_option = 0;
	
	if(self.hex.slider[self.hex.current_menu][self.current_option] > self.hex.slider_max[self.hex.current_menu][self.current_option])
        self.hex.slider[self.hex.current_menu][self.current_option] = self.hex.slider_min[self.hex.current_menu][self.current_option];

    if(self.hex.slider[self.hex.current_menu][self.current_option] < self.hex.slider_min[self.hex.current_menu][self.current_option])
        self.hex.slider[self.hex.current_menu][self.current_option] = self.hex.slider_max[self.hex.current_menu][self.current_option];
	
	self menuStructure();

    if(!isDefined(self.hex.text[self.hex.current_menu][self.current_option - 7]) || self.hex.text[self.hex.current_menu].size <= 8)
    {
        for(i = 0; i < 8; i++)
        {
            if(isDefined(self.hex.text[self.hex.current_menu][i]))
                self.ui.text[i] setTextWrap(self.hex.text[ self.hex.current_menu ][ i ]);
            else
                self.ui.text[i] setTextWrap("");

            if(isDefined(self.hex.bool[self.hex.current_menu][i]))
                self.ui.bool_text[i] setTextWrap("[" + self.hex.bool[self.hex.current_menu][i] + "]");
            else
                self.ui.bool_text[i] setTextWrap("");
        }
    }
    else
    {
        optnum = 0;
        for(i = self.current_option - 7; i < self.current_option + 8; i++)
        {  
            if(isDefined(self.hex.text[self.hex.current_menu][i]))
                self.ui.text[optnum] setTextWrap(self.hex.text[self.hex.current_menu][i]);
            else
                self.ui.text[optnum] setTextWrap("");

			if(isDefined(self.hex.bool[self.hex.current_menu][i]))
                self.ui.bool_text[optnum] setTextWrap("[" + self.hex.bool[self.hex.current_menu][i] + "]");
            else
                self.ui.bool_text[optnum] setTextWrap("");

            optnum ++;
        }
    }
	
    if(self.current_option < 8)
        scroll = self.current_option;
    else
        scroll = 7;

	self.ui.scrollbar.y = (self.hex.y + 42) + (18 * scroll);

    self.ui.options setTextWrap("[" + (self.current_option + 1) + ", " + self.hex.option_count[self.hex.current_menu]  + "]");
}

createMenuText()
{
	for(i = 0; i < 8; i++)
	{
		self.ui.text[i] = self createText("objective", 1, "LEFT", "TOP", self.hex.x - ((self.hex.width / 2) - 3), self.hex.y + 42 + (18 * i), 5, (1, 1, 1), 2, (0, 0, 0), 1, self.hex.text[self.hex.current_menu][i]);
		self.ui.bool_text[i] = self createText("objective", 1, "RIGHT", "TOP", self.hex.x + (self.hex.width / 2) - 3, self.hex.y + 42 + (18 * i), 5, (1, 1, 1), 1, (0, 0, 0), 1, self.hex.bool[self.hex.current_menu][i]);
	}
}

destroyMenuText()
{
	if(isDefined(self.ui.text))
	{
		for(i = 0; i < 8; i++)
		{
			self.ui.text[i] destroy();
			self.ui.bool_text[i] destroy();
		}
	}
}

createHud()
{
	self.ui.title = createText("objective", 1, "LEFT", "TOP", self.hex.x - ((self.hex.width / 2) - 3), self.hex.y + 20, 5, (1, 1, 1), 1, (0, 0, 0), 1, "");
	self.ui.title.foreground = true;
    self.ui.options = createText("objective", 1, "RIGHT", "TOP", self.hex.x + (self.hex.width / 2) - 3, self.hex.y + 20, 5, (1, 1, 1), 1, (0, 0, 0), 1, "ssss");
	self.ui.options.foreground = true;
	self.ui.header = createRectangle("TOP", "TOP", self.hex.x, self.hex.y, self.hex.width, self.hex.header_height, self.hex.colour, 0.8, 0, self.hex.header_shader);
    self.ui.background = createRectangle("TOP", "TOP", self.hex.x, self.hex.y + self.hex.header_height, self.hex.width, 150, (0, 0, 0), 0.7, 0, "white");
	self.ui.scrollbar = createRectangle("CENTER", "TOP", self.hex.x, self.hex.y + 42, self.hex.width, self.hex.scrollbar_height, self.hex.colour, 0.8, 1, "white");
	self createMenuText();
}

destroyHud()
{
	self.ui.title destroy();
    self.ui.options destroy();
	self.ui.background destroy();
	self.ui.header destroy();
	self.ui.scrollbar destroy();
}

loadMenu(menu)
{
	self.hex.lastscroll[self.hex.current_menu] = self.current_option;
    self.hex.current_menu = menu;
    if(!isDefined(self.hex.lastscroll[self.hex.current_menu]))
        self.current_option = 0;
    else
        self.current_option = self.hex.lastscroll[self.hex.current_menu];

	self.ui.title setTextWrap(self.hex.title[self.hex.current_menu]);
	self updateScroller();
	self updatebackground();
}

updatebackground()
{
    amount = self.hex.text[self.hex.current_menu].size;
    if(amount > 8)
        amount = 8;
    self.ui.background setShader("white", self.hex.width, 6 + (18 * amount));
}

setPers(key, value)
{
    self.pers[key] = value;
}

getPers(key)
{
    return self.pers[key];
}

createMenu(menu, title, parent)
{
	self.hex.title[menu] = title;
	self.hex.parent[menu] = parent;
	self.hex.option_count[menu] = 0;
}

addOption(menu, text, func, input, input2, input3, input4)
{
    index = self.hex.option_count[menu];
	self.hex.text[menu][index] = text;
	if(isDefined(func) && func == ::loadMenu)
    self.hex.bool[menu][index] = ">";
	self.hex.function[menu][index] = func;
	self.hex.a1[menu][index] = input;
	self.hex.a2[menu][index] = input2;
	self.hex.a3[menu][index] = input3;
	self.hex.a4[menu][index] = input4;
	self.hex.option_count[menu] += 1;
}

addVar(menu, text, func, bool, input, input2, input3, input4)
{
    index = self.hex.option_count[menu];
    self.hex.text[menu][index] = text;
	self.hex.bool[menu][index] = bool;
    self.hex.function[menu][index] = func;
    self.hex.a1[menu][index] = input;
	self.hex.a2[menu][index] = input2;
	self.hex.a3[menu][index] = input3;
	self.hex.a4[menu][index] = input4;
    self.hex.option_count[menu] += 1;
}

addBool(menu, text, func, bool, input, input2, input3, input4)
{
    index = self.hex.option_count[menu];
    self.hex.text[menu][index] = text;
	if(isDefined(bool))
    {
        if(bool)
            self.hex.bool[menu][index] = "^2On^7";
        else if(!bool)
            self.hex.bool[menu][index] = "^1Off^7";
    }
    self.hex.function[menu][index] = func;
    self.hex.a1[menu][index] = input;
	self.hex.a2[menu][index] = input2;
	self.hex.a3[menu][index] = input3;
	self.hex.a4[menu][index] = input4;
    self.hex.option_count[menu] += 1;
}

addStringSlider(menu, text, func, string, stringname, input2)
{
    index = self.hex.option_count[menu];
    if (!isDefined(self.hex.slider[menu][index]))
    self.hex.slider[menu][index] = 0;
	
    self.hex.slider_min[menu][index] = 0;
    self.hex.slider_max[menu][index] = string.size - 1;
    self.hex.slider_move[menu][index] = 1;
    self.hex.slider_type[menu][index] = "string";
    self.hex.text[menu][index] = text;
    self.hex.bool[menu][index] = "<" + stringname[self.hex.slider[menu][index]] + ">";
    self.hex.function[menu][index] = func;
    self.hex.a1[menu][index] = string[self.hex.slider[menu][index]];
    self.hex.a2[menu][index] = input2;
    self.hex.option_count[menu] += 1;
}

addDvarSlider(menu, text, min, max, step, dvar)
{
    index = self.hex.option_count[menu];
    if (isDefined(dvar) && getDvar(dvar) != "")
        self.hex.slider[menu][index] = getDvarFloat(dvar);
    else
        self.hex.slider[menu][index] = min;
		
    self.hex.slider_min[menu][index] = min;
    self.hex.slider_max[menu][index] = max;
    self.hex.slider_move[menu][index] = step;
    self.hex.slider_dvar[menu][index] = dvar;
    self.hex.slider_type[menu][index] = "dvar";
    self.hex.text[menu][index] = text;
    self.hex.bool[menu][index] = "<" + self.hex.slider[menu][index] + ">";
    self.hex.option_count[menu] += 1;
}

addBindSlider(menu, text, action_func, stop_endon, crouchonly)
{
    buttons = strTok("Unbound,[{+actionslot 1}],[{+actionslot 2}],[{+actionslot 3}],[{+actionslot 4}]", ",");
    self addStringSlider(menu, text + " [{+usereload}]", ::genericBind, buttons, buttons);

    index = self.hex.option_count[menu] - 1;
    self.hex.a2[menu][index] = action_func;
    self.hex.a3[menu][index] = stop_endon;
    self.hex.a4[menu][index] = crouchonly;

    // self-register so restore_all_binds() finds it on spawn
    // with zero manual bookkeeping
    if(!isDefined(self.bind_configs))
        self.bind_configs = [];

    config = spawnstruct();
    config.action_func = action_func;
    config.stop_endon   = stop_endon;
    config.crouchonly = crouchonly;
    self.bind_configs[self.bind_configs.size] = config;
}

genericBind(bind, action_func, stop_endon, crouchonly)
{
    self notify(stop_endon);
    self.pers[stop_endon] = bind;
    self endon(stop_endon);
    self endon("disconnect");
    self endon("death");

    if(bind == "Unbound")
        return;

    for(;;)
    {
        pressed = false;

        if(!self.hex.is_open)
        {
            if((crouchonly && self getStance() == "crouch") || !crouchonly)
            {
                if(bind == "[{+actionslot 1}]" && self actionslotOneButtonPressed())
                    pressed = true;
                else if(bind == "[{+actionslot 2}]" && self actionslotTwoButtonPressed())
                    pressed = true;
                else if(bind == "[{+actionslot 3}]" && self actionslotThreeButtonPressed())
                    pressed = true;
                else if(bind == "[{+actionslot 4}]" && self actionslotFourButtonPressed())
                    pressed = true;
            }
        }

        if(pressed)
        {
            self thread [[action_func]](self);
            wait 0.4;
        }
        else
        {
            wait 0.05;
        }
    }
}

createText(font, fontscale, align, relative, x, y, sort, color, alpha, glowColor, glowAlpha, text)
{
	textElem = createFontString( font, fontscale );
	textElem setPoint( align, relative, x, y );
	textElem.sort = sort;
	textElem.type = "text";
	textElem setTextWrap(text);
	textElem.color = color;
	textElem.alpha = alpha;
	textElem.glowColor = glowColor;
	textElem.glowAlpha = glowAlpha;
	textElem.hideWhenInMenu = false;
	return textElem;
}

createRectangle(align, relative, x, y, width, height, color, alpha, sorting, shadero)
{
	barElemBG = newClientHudElem(self);
	barElemBG.elemType = "bar";

	barElemBG.width = width;
	barElemBG.height = height;
	barElemBG.align = align;
	barElemBG.relative = relative;
	barElemBG.xOffset = 0;
	barElemBG.yOffset = 0;
	barElemBG.children = [];
	barElemBG.color = color;
	if(isDefined(alpha))
		barElemBG.alpha = alpha;
	else
		barElemBG.alpha = 1;
	barElemBG setShader( shadero, width , height );
	barElemBG.hidden = false;
	barElemBG.sort = sorting;
	barElemBG setPoint(align,relative,x,y);
	return barElemBG;
}

addString(string)
{
	level.strings[level.strings.size] = string;
    level notify("string_added");
}

fixString()
{
	self notify("new_string");
    self endon("new_string");
    while(isDefined(self)) 
    {
        level waittill("overflow_fixed");
        self setTextWrap(self.string);
    }
}

initialiseOverflowFix()
{
	level.strings = [];
    level.overflowElem = createServerFontString("default", 1.5);
    level.overflowElem setTextWrap("overflow");
    level.overflowElem.alpha = 0;
	level thread overflowFixMonitor();
}

overflowFixMonitor()
{
	for(;;) 
    {
        level waittill("string_added");
        if(level.strings.size >= 45) 
        {
            level.overflowElem clearAllTextAfterHudElem();
            level.strings = [];
            level notify("overflow_fixed");
        }
        wait 0.05;
    }
}

setTextWrap(text)
{
	self.string = text;
    self setText(text);
    self thread fixString();
    self addString(text);
}

uniPers(key, value)
{
    if(!isdefined(self.pers[key]))
    {
        self.pers[key] = value;
    }
}

remember(key, func, input, input2)
{
    if(self.pers[key])
        self thread [[func]](input, input2);
}

setDvarIfUni(dvar, value)
{
    if(!isDefined(self.pers[dvar]))
    {
        setDvar(dvar, value);
        self.pers[dvar] = true;
    }
}

rememberBinds()
{
    self endon("disconnect");
    self endon("death");

    if(!isDefined(self.bind_configs))
        return;

    for(i = 0; i < self.bind_configs.size; i++)
    {
        config = self.bind_configs[i];
        if(isDefined(self.pers[config.stop_endon]))
            self thread genericBind(self.pers[config.stop_endon], config.action_func, config.stop_endon, config.crouchonly);
    }
}

variables()
{
    uniPers("loadOnSpawn", true);
    setDvarIfUni("locationIndex", 0);
    uniPers("ufoMode", false);
    uniPers("resetVelocity", true);
    uniPers("stopSaveBind", 0);
    uniPers("stopLoadBind", 0);

    uniPers("aimbot", true);
    uniPers("aimbotWeapon", "none");
    setDvarIfUni("aimbotDistance", 0);
    uniPers("midAirOnly", false);
    setDvarIfUni("aimbotDelay", 0);
    uniPers("hitLocation", "head");

    uniPers("godMode", false);

    uniPers("loadAmmoOnSpawn", true);

    if(self.pers["godMode"])
        self EnableInvulnerability();

    self remember("loadOnSpawn", ::loadLocation, self);

    self remember("aimbot", ::monitorAimbot);

    //self remember("loadAmmoOnSpawn", ::loadAmmoOnSpawnAll);

    if(self isHost())
    {
        uniPers("botsFreeze", true);
        self thread freezeBotsLoop();
    }

    self thread alwaysChangeClass();
    self thread rememberBinds();
}

menuStructure()
{
    /*addOption(menu, text, func, input, input2, input3, input4)
    addBool(menu, text, func, bool, input, input2)
    addStringSlider(menu, text, func, string , stringname)
    addDvarSlider(menu, text, min, max, step, dvar)
    */

	self createMenu("main", "\\o/", "close_menu");
    self addOption("main", "Location Menu", ::loadMenu, "Location Menu");
    self addOption("main", "Aimbot Menu", ::loadMenu, "Aimbot Menu");
    self addOption("main", "Miscellaneous Menu", ::loadMenu, "Miscellaneous");
    self addOption("main", "Binds Menu", ::loadMenu, "Binds Menu");
    self addOption("main", "Bots Menu", ::loadMenu, "Bots Menu");
    self addOption("main", "Game Settings", ::loadMenu, "Game Settings");
	self addOption("main", "Players Menu", ::loadMenu, "Players Menu");

    self createMenu("Location Menu", "Location Menu", "main");
    self addBool("Location Menu", "Load on Spawn", ::loadOnSpawn, self.pers["loadOnSpawn"]);
    self addDvarSlider("Location Menu", "Location Index", 0, 9, 1, "locationIndex");
    self addOption("Location Menu", "Save Location", ::saveLocation, self);
    self addOption("Location Menu", "Load Location", ::loadLocation, self);
    self addBool("Location Menu", "Reset Velocity", ::resetVelocity, self.pers["resetVelocity"]);
    self addBindSlider("Location Menu", "Save Bind", ::saveLocation, "stopSaveBind", 1);
    self addBindSlider("Location Menu", "Load Bind", ::loadLocation, "stopLoadBind", 1);

    self createMenu("Aimbot Menu", "Aimbot Menu", "main");
    self addBool("Aimbot Menu", "Aimbot", ::aimbot, self.pers["aimbot"]);
    self addVar("Aimbot Menu", "Aimbot Weapon", ::aimbotWeapon, self.pers["aimbotWeapon"]);
    self addDvarSlider("Aimbot Menu", "Aimbot Distance", 0, 10000, 50, "aimbotDistance");
    self addBool("Aimbot Menu", "Mid Air Only", ::midAirOnly, self.pers["midAirOnly"]);
    self addStringSlider("Aimbot Menu", "Hit Location", ::aimbotLocation, strTok("head,body", ","), strTok("head,body", ","));

    self createMenu("Miscellaneous", "Miscellaneous Menu", "main");
    self addOption("Miscellaneous", "Self", ::loadMenu, "Self");
    self addOption("Miscellaneous", "Weapons", ::loadMenu, "Weapons");
    self addOption("Miscellaneous", "Killstreaks", ::loadMenu, "Killstreaks");

        self createMenu("Self", "Self Menu", "Miscellaneous");
        self addBool("Self", "God Mode", ::godMode, self.pers["godMode"]);

        self createMenu("Weapons", "Weapons Menu", "Miscellaneous");
        self addOption("Weapons", "Drop Current", ::dropCurrentWeapon);
        self addOption("Weapons", "Save Ammo", ::saveAmmo);
        self addOption("Weapons", "Load Ammo", ::loadAmmo);
        //self addBool("Weapons", "Load Ammo on Spawn", ::loadAmmoOnSpawn, self.pers["loadAmmoOnSpawn"]);

        self createMenu("Killstreaks", "Killstreaks Menu", "Miscellaneous");
        //self addOption("Killstreaks", "Give All Killstreaks", ::giveAllStreaks);

    self createMenu("Binds Menu", "Binds Menu", "main");

    self createMenu("Bots Menu", "Bots Menu", "main");
    if(self isHost())
        self addBool("Bots Menu", "Freeze Bots", ::botsFreeze, self.pers["botsFreeze"]);
    self addOption("Bots Menu", "Spawn Enemy", ::spawnBot, getOtherTeam(self.pers["team"]));
    self addOption("Bots Menu", "Spawn Friendly", ::spawnBot, self.pers["team"]);

    self createMenu("Game Settings", "Game Settings", "main");
    self addOption("Game Settings", "Dvar List", ::loadMenu, "Dvar List");

        self createMenu("Dvar List", "Dvar List", "Game Settings");
        self addDvarSlider("Dvar List", "timescale", 0.1, 2, 0.1, "timescale");
        self addDvarSlider("Dvar List", "bg_gravity", 0, 800, 50, "bg_gravity");

    self createMenu("Players Menu", "Players Menu", "main");
    for(i = 0; i < level.players.size; i++)
    {
        player = level.players[i];
        self addVar("Players Menu", player.name, ::loadMenu, player.verified, player.name + "menu");

        self createMenu(player.name + "menu", player.name, "Players Menu");
        self addBool(player.name + "menu", "Menu Access", ::verifyClient, player.has_menu, player);
        self addOption(player.name + "menu", "Kick Player", ::kickPlayer, player);
        self addOption(player.name + "menu", "Kill Player", ::killPlayer, player);
        self addOption(player.name + "menu", "Teleport Player", ::teleportPlayer, player);
        self addOption(player.name + "menu", "Save Location", ::saveLocation, player);
        self addOption(player.name + "menu", "Load Location", ::loadLocation, player);
        self addBool(player.name + "menu", "Freeze Player", ::freezePlayer, player.pers["frozen"], player);
        self addBool(player.name + "menu", "God Mode", ::playerGodMode, player.pers["god"], player);
        self addStringSlider(player.name + "menu", "Set Stance", ::playerSetStance, strTok("prone,crouch,stand", ","), strTok("prone,crouch,stand", ","), player);
    }
}

godMode()
{
    self.pers["godMode"] = !self.pers["godMode"];
    if(self.pers["godMode"])
        self EnableInvulnerability();
    else
        self DisableInvulnerability();
}

verifyClient(player)
{
    if(!player isHost())
    {
        if(!player.has_menu)
        {
            player.has_menu = true;
            player.verified = "^2Verified^7";
            player thread initialiseMenu();
        }
        else 
        {
            player.has_menu = false;
            player.verified = "^1Unverified^7";
            player thread destroyMenuText();
            player thread destroyHud();
            player.hex.is_open = false;
        }
    }
}

saveLocation(player)
{
    player.pers["location"][getDvarInt("locationIndex")] = player getOrigin();
    player.pers["angle"][getDvarInt("locationIndex")] = player getPlayerAngles();
}

loadLocation(player)
{
    if(player.pers["resetVelocity"])
        player freezeControls(true);

    player setOrigin(player.pers["location"][getDvarInt("locationIndex")]);
    player setPlayerAngles(player.pers["angle"][getDvarInt("locationIndex")]);

    if(player.pers["resetVelocity"])
    {
        wait 0.3;
        player freezeControls(false);
    }
}

loadOnSpawn()
{
    self.pers["loadOnSpawn"] = !self.pers["loadOnSpawn"];
}

resetVelocity()
{
    self.pers["resetVelocity"] = !self.pers["resetVelocity"];
}

botsFreeze()
{
    self.pers["botsFreeze"] = !self.pers["botsFreeze"];
    if(!self.pers["botsFreeze"])
    {
        for(i = 0; i < level.players.size; i++)
        {
            player = level.players[i];
            if(player.pers["isBot"] && isDefined(player.pers["isBot"]))
            {
                player freezeControls(false);
                self notify("stop_freeze_bots");
            }
        }
    }
    else 
        self thread freezeBotsLoop();
}

spawnBot(team)
{
    bot = addtestclient();

    if(isDefined(bot))
    {
        bot.pers["isBot"] = true;

        if(team != "autoassign")
            bot.pers["team"] = team;

        bot thread maps/mp/gametypes/_bot::bot_spawn_think(team);
        bot thread botSetup();
    }

    return;
}

botSetup()
{
    self endon("disconnect");

    self waittill("spawned_player");

    self DisableInvulnerability();
    self.maxhealth = 100;
    self.health = self.maxhealth;
}

freezeBotsLoop()
{
    self endon("disconnect");
    self endon("stop_freeze_bots");
    while(self.pers["botsFreeze"])
    {
        for(i = 0; i < level.players.size; i++)
        {
            player = level.players[i];
            if(player.pers["isBot"] && isDefined(player.pers["isBot"]))
            {
                player freezeControls(true);
            }
        }
        wait 0.05;
    }
}

aimbotWeapon()
{
    self.pers["aimbotWeapon"] = self getCurrentWeapon();
}

midAirOnly()
{
    self.pers["midAirOnly"] = !self.pers["midAirOnly"];
}

aimbotLocation(location)
{
    self.pers["hitLocation"] = location;
}

aimbot()
{
    self.pers["aimbot"] = !self.pers["aimbot"];
    if(self getPers("aimbot"))
        self thread monitorAimbot();
    else 
        self notify("endAimbot");
}

monitorAimbot()
{
    self endon("endAimbot");
    self endon("disconnect");
    self endon("death");
    for(;;)
    {
        self waittill("weapon_fired");
        
        aimAt = undefined;

        start = self geteye();
        end = start + anglestoforward(self getplayerangles()) * 1000000;
        bullet = bulletTrace(start, end, false, self)["position"];

        for(i = 0; i < level.players.size; i++)
        {
            player = level.players[i];
            if((player == self) || (!isAlive(player)) || (level.teamBased && self.pers["team"] == player.pers["team"]))
                continue;
            else 
                aimAt = player;
        }
        if(self getCurrentWeapon() == self getPers("aimbotWeapon"))
        {
            if(level.teamBased && aimAt.pers["team"] != self.pers["team"] && aimAt != self)
            {
                if(distance(aimAt.origin, bullet) < getDvarInt("aimbotDistance"))
                {
                    if(self getPers("midAirOnly") && !self isOnGround() || !self getPers("midAirOnly")) 
                    {
                        aimAt thread [[level.callbackPlayerDamage]]( self, self, 500, 8, "MOD_RIFLE_BULLET", self getcurrentweapon(), ( 0, 0, 0 ), ( 0, 0, 0 ), self.pers["hitLocation"], 0, 0 );
                    }
                }
            }
        }
    }
}

kickPlayer(player)
{
    kick(player getEntityNumber());
}

killPlayer(player)
{
    player suicide();
}

giveAllStreaks()
{
    //self maps\mp\gametypes\_globallogic_score::_setplayermomentum(self, 1600);
}

teleportPlayer(player)
{
    start = self geteye();
    end = start + anglestoforward(self getplayerangles()) * 1000000;
    bullet = bulletTrace(start, end, false, self)["position"];

    player setOrigin(bullet);
}

alwaysChangeClass()
{
    self endon("disconnect");
    self endon("death");
    for(;;)
    {
        self waittill("changed_class");
        self maps\mp\gametypes\_class::setclass(self.pers["class"]);
        self.tag_stowed_back = undefined;
        self.tag_stowed_hip = undefined;
        self maps\mp\gametypes\_class::giveloadout(self.pers["team"], self.pers["class"]);
        self maps\mp\gametypes\_hardpoints::giveownedkillstreak();

        for(i = 0; i < 10; i++)
            self iPrintLnBold(" ");
    }
}

dropCurrentWeapon()
{
    self dropItem(self getCurrentWeapon());
}

changeMapWrap(mapname)
{
    map(mapname);
}

customMode(custommode)
{
    /*
    switch(custommode)
    {
        case "2H2B":
            setDvar("ui_gametype", "dm");
            setDvar("gameType", "dm");
            setDvar("custom_mode", "2H2B");
            map(getDvar("ui_mapname"));
            break;
    }*/
}

freezePlayer(player)
{
    player.pers["frozen"] = !player.pers["frozen"];
    if(player.pers["frozen"])
        player freezeControls(true);
    else
        player freezeControls(false);
}

playerGodMode(player)
{
    player.pers["god"] = !player.pers["god"];
    if(player.pers["god"])
        player EnableInvulnerability();
    else
        player DisableInvulnerability();
}

playerSetStance(stance, player)
{
    player endon("death");
    player endon("disconnect");
    for(;;)
    {
        player setStance(stance);
        wait 0.05;
    }
}

saveAmmo()
{
    self.pers["saveAmmoClip" + self getCurrentWeapon()] = self getWeaponAmmoClip(self getCurrentWeapon());
    self.pers["saveAmmoStock" + self getCurrentWeapon()] = self getWeaponAmmoStock(self getCurrentWeapon());
}

loadAmmo()
{
    if(isDefined(self.pers["saveAmmoClip" + self getCurrentWeapon()]))
    {
        self setWeaponAmmoClip(self getCurrentWeapon(), self.pers["saveAmmoClip" + self getCurrentWeapon()]);
        self setWeaponAmmoStock(self getCurrentWeapon(), self.pers["saveAmmoStock" + self getCurrentWeapon()]);
    }
    else 
        self iPrintLn("Error: ^1No Ammo Saved");
}

loadAmmoOnSpawn()
{
    self.pers["loadAmmoOnSpawn"] = !self.pers["loadAmmoOnSpawn"];
}

loadAmmoOnSpawnAll()
{
    /*
    primary = self getLoadoutWeapon(self.class_num, "primary");
    self setWeaponAmmoClip(primary, self.pers["saveAmmoClip" + primary]);
    self setWeaponAmmoStock(primary, self.pers["saveAmmoStock" + primary]);

    secondary = self getLoadoutWeapon(self.class_num, "secondary");
    self setWeaponAmmoClip(secondary, self.pers["saveAmmoClip" + secondary]);
    self setWeaponAmmoStock(secondary, self.pers["saveAmmoStock" + secondary]);
    */
}