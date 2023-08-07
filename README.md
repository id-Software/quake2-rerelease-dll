# Quake II Rerelease Game Source

This repository contains the game code for the 2023 rerelease of Quake II, for users who wish to mod the game, along with the original game code that was use for reference. Mods can be loaded into the rerelease the same way as the original game: launch the game with `+set game mymod` or type `game mymod` into the console while the game is running. We recommend installing mods into your `%USERPROFILE%\Saved Games\Nightdive Studios\Quake II` directory to ensure the original game files do not get modified.

id Software is unable to provide support for this release, however we urge you to take advantage of the depth of community-driven resources already available.

The rerelease of Quake II uses a new version of the API to communicate between the server & the game module. It also introduces a very thin "client game" module, akin to Quake III Arena's cgame module, to allow for extended modding opportunities that change previously hardcoded client behavior. It also has a new network protocol, version 2023.

This codebase is a combination of the separate game modules that were part of the original game: baseq2, ctf, rogue, and xatrix. It requires a C++17 compiler. In cases of conflicting spawnflags, maps were modified in order to resolve issues, so original expansion pack maps may not load correctly with this DLL. The combined FGD as used for development is also available for users wishing to make new maps. A modified TrenchBroom Quake II `gameconfig.cfg` is included as there are modified textureflags.

Because the game export interface has changed, existing mods may be able to be moved over to support the API changes. However, in order to support all expansion packs under one codebase and new features in the rerelease, there have been some major changes to structure and layout, so old mods wishing to use the new codebase may need to be rewritten.

The API changes discussed here are written from the perspective of the game DLL.

## Compiling

The game DLL has only been tested with Clang, VS2019 and VS2022.

The code can compile under both C++17 and C++20. Using C++20 allows you to skip `fmtlib` as a dependency.

### Required preprocessor definitions
* `GAME_INCLUDE` must be defined, tells `game.h` that this is the game DLL compiling it.
* `KEX_Q2GAME_EXPORTS` must be defined, tells `game.h` that we are exporting the `GetGameAPI` function.
* `KEX_Q2GAME_DYNAMIC` must be defined. The game DLL supports static linking on console platforms, but is always dynamic on PC.
* `NO_FMT_SOURCE`: this is only here because of a limitation in the internal build system. It must be defined.

### Optional preprocessor definitions
* `KEX_Q2_GAME`: must be defined if compiling for Kex. This changes the behavior of the `say` command to go through the lobby.
* `KEX_Q2GAME_IMPORTS`: only used by engine, tells `game.h` that we are importing `GetGameAPI`.
* `USE_CPP20_FORMAT`: if explicitly defined, C++20's `<format>` library will be used instead of `fmtlib`; otherwise `fmtlib` usage will be autodetected.


### Dependencies
* [fmtlib](https://github.com/fmtlib/fmt): If `USE_CPP20_FORMAT` is not set, the library needs to be available in the `fmt` subdirectory.
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp): Must be placed inside `json` subdirectory.

Both of these can also be installed via vcpkg: `vcpkg install jsoncpp:x64-windows fmt:x64-windows`

### Windows (Visual Studio 2019 / 2022):
* We recommend placing the source in a subfolder within a mod directory. For example, alongside `baseq2`, make a folder called `mymod`, enter that folder, make a folder called `src`, and copying the contents of the `rerelease` directory into the newly-created `src` subfolder.
* Open `game.sln`
* Build solution

Debugging the DLL is possible when attaching to the engine EXE. Note that if you are using VS2022 Hot Reload, due to an internal Hot Reload issue, current edits will be lost when disconnecting from the server, or changing levels using the `map` command.

## 40hz Tickrate Support

As part of this release, all internal logic in the game DLL has been adjusted to run at 40hz compared to the original 10hz of the original engine. This provides a better gameplay experience, and allows maps and game logic to run at more precise steps than the original 100ms. 40hz was chosen as it is a multiple of the original 10hz, operates at an integral 25ms step, and was the best balance between bandwidth and CPU concerns around the original tech.

## Print Adjustments

As part of the API cleanup, the game DLL no longer uses varargs in any of its functions. Varargs are compiler-specific and not very portable, so instead, the onus is on the caller to handle formatting. As a bonus, this allows the game DLL to more easily hook in modern formatting providers; the game DLL uses `fmt` almost exclusively. Several built-in types, like `edict_t` and `vec3_t`, can be formatted directly.

## Math Changes

Since C++ is now used in the game DLL, math functions were made constexpr where appropriate, and operator overloads are used to make math easier to work with and closer to QuakeC. For instance, `VectorMA(a, s, b, c)` can now be written as `c = a + (b * s)`, which expresses the operation better.

## Type Changes

`qboolean`, which was aliased to `int32_t`, is now instead aliased to `bool`. This type should be equivalent to C's `_Bool`.

## Info Keys

In the original Quake II, all infokey values are purely ASCII with upper bits stripped. Kex and the Quake II rerelease engine supports UTF-8 for things like player names, which necessated a change to the way info keys work. Instead of implementing a whole UTF-8 system in the game DLL, these functions are now imports, so the engine is in control of the parsing and string management.

Userinfo variables are now suffixed with a number for split screen support. For instance, `name` and `skin` have become `name_0` and `skin_0` for the first player, `name_1` and `skin_1` for the second player, and so on.

## Extensions

In an attempt to remain compatible with future community engines, all engine interfaces contain stubbed functions for GetExtension. This is currently unused and will only return nullptr, however other engines may wish to support returning named structs containing extra function pointers based on string parameters. This is similar to `getextension` that has become standard in many QuakeC environments.

Conforming engines should return nullptr if an extension is not supported, otherwise they should return a non null pointer value that is relevant to the requested feature. Supporting engines should use namespaces for features to prevent name collisions around common and possibly incompatible implementations.

## Player Movement

Player movement ("pmove") is now handled as an export in both `game_export_t` *and* `cgame_export_t`. This allows a game module to modify physics while still working with client prediction. Pmove also received several upgrades, such as more bits for flags and full float positioning.

Because a lot of movement quirks in Quake II were indirectly caused by the compression, these behaviors were retained. Trick jumping is now an explicit movement type, to allow for things like the Mega Health box jumps to still work. Some fixes were made, like jumping higher below the 0 z point of the map.

## Frame visibility

As part of network improvements, some changes were made to the "entity is visible to client in frame" methods:
* Split-screen support since all clients share a frame.
* Entities with shadows will be visible if their shadows may be visible to a player.
* Sound attenuation culling is now calculated formulaically to match the sound system, and takes loop_attenuation into account.

## Entity linkage

To fix a legacy bug where lasers only relied on one position for culling, RF_BEAM entities now set their `absmin` & `absmax` properly. This can cause them to be a bit inflated if they are angled, but should not cause any issues otherwise.

In a similar vein, `gi.linkentity` no longer automatically sets `old_origin` for beams. This is to make it a bit easier to work with beams, as otherwise you'd be forced to link twice to get it linked into the world correctly. This *might* break old mods that depends on this behavior.

## Audio positioning

Entity spatialization underwent an overhaul (`CL_GetEntitySoundOrigin` mainly).

* Brush models will use the closest point on the bmodel's absmin/absmax to the listener's origin. This allows moving brush models with sounds to make consistent sounds, and be full volume if you are inside of them.
* Beams now support `s.sound`, and plays their sound on the nearest point between the two beam origins.

As a secondary fix to the above, `S_StartSound` has slightly different logic now surrounding what origin to pick when playing a sound:

```cpp
if (entnum < MAX_EDICTS && (origin || fixed_origin))
{
	// [Paril-KEX] if we can currently see the entity in question
	// and it's a bmodel or beam, don't use a fixed origin so it sounds correct
	if (!fixed_origin && entnum > 1 && (cl_entities[entnum].serverframe == cl.frame.serverframe || 
		(cl_entities[entnum].current.solid == PACKED_SOLID_BSP || (cl_entities[entnum].current.renderfx & RF_BEAM))))
	{
		ps->fixed_origin = false;
	}
	else
	{
		VectorCopy (origin, ps->origin);
		ps->fixed_origin = true;
	}
}
else
	ps->fixed_origin = false;
```

`fixed_origin` is set to `flags & SND_EXPLICIT_POS` for svc_sound packets, and is `false` otherwise. If the playsounds' `fixed_origin` field is set, then the `ps->origin` value will *always* be used over automatically trying to determine its position.

## Client entity adjustments

* Beam origin points are interpolated if they exist between frames.
* `TE_ELECTRIC_SPARKS` and `TE_SCREEN_SPARKS`/`TE_SHIELD_SPARKS` will only play sounds once on any given frame.
* Entities will never play the same footstep sound twice in a row.
* Beams now squash the ends of their beams so they don't intersect walls or end early.
* Alpha and transparency settings now get copied over to all sub-modelindices.
* Lightramps are now interpolated, which looks nicer and helps with epilepsy.
* `delta_angles` are interpolated now, although this is never used at all in the game.
* `screen_blend` and `damage_blend` are interpolated now, but only if the frame prior didn't have a clear color.

## Configstrings

Configstrings have been overhauled. There is now a theoretical maximum of `32767` entries, although you are still bound by the game APIs value of `MAX_CONFIGSTRINGS`.

The maximum length of a configstring has increased from `64` to `96`.

The API now canonizes that certain spans (`CS_STATUSBAR` and `CS_GENERAL`) can span multiple lines. A `CS_SIZE` function is provided to calculate the total size (in bytes) that can be written to for a given configstring ID.

A convenience function, `CS_REMAP`, is provided to help remap old configstring IDs to new ones. This is used in our engine to provide old demo support.

`MAX_MODELS`, `MAX_SOUNDS` and `MAX_IMAGES` have been increased from 256 to 8192, 2048, and 512, respectively.

### CS_STATUSBAR

This entry now spans entries 5 to 58 instead of 5 to 28, giving you an effective size of 5184 bytes (minus one for the null terminator) for the statusbar strings, up from 1536 bytes.

### CS_SHADOWLIGHTS

This new span each consists of a shadow light entry. Its format is semicolon-separated numerical values, in the following type & order:

* int entity_order
* int type (0 = point, 1 = cone)
* float radius
* int resolution
* float intensity
* float fade_start
* float fade_end
* int lightstyle
* float coneangle
* float direction_x
* float direction_y
* float direction_z

### CS_WHEEL_WEAPONS

This new span consists of entries for the weapon wheel. It consists of pipe-separated integral values, in the following order:

* CS_ITEMS item index
* CS_IMAGES image index
* CS_WHEEL_AMMO ammo index (or -1 for no ammo)
* minimum ammo to use weapon
* whether the weapon is on the powerup wheel or the weapon wheel
* additional sort integer
* quantity to warn on low ammo on
* whether this weapon is droppable or not

### CS_WHEEL_AMMO

This new span consists of entries for the weapon wheel ammo types. It consists of pipe-separated integral values, in the following order:

* CS_ITEMS item index
* CS_IMAGES image index

### CS_WHEEL_POWERUPS

This new span consists of entries for the powerup wheel. It consists of pipe-separated integral values, in the following order:

* CS_ITEMS image index
* CS_IMAGES image index
* if 1, it is a togglable powerup instead of having a count
* additional sort integer
* whether we can drop this powerup or not
* CS_WHEEL_AMMO ammo index, if applicable (-1 for no ammo)

### CS_CD_LOOP_COUNT

Integer which determines how many times to loop the music before switching to the ambient track. Leave blank to use the clients' preferred value, otherwise it is forced to this value (a value of zero means never switch to ambient track).

### CS_GAME_STYLE

Inform the client about the type of game being played.

# Structures

Quake II has two main shared structures: `gclient_t` and `edict_t`. These split off into various other shared structures that have to be the same between the game & server.

Like the original release, the "shared" aspects of these structs must be identical, and are stored in `game.h` as `edict_shared_t` and `gclient_shared_t` respectively.

The structure changes will be listed from the bottom-up. Full listings of the structures can be found in the source.

## cvar_flags_t

### CVAR_USER_PROFILE (bit 5)

This is a new flag that is solely for the client; it indicates that a cvar is treated like userinfo for the purposes of storage, but is not sent to the server like userinfo usually is. For example, this flag is applied to `cl_run_N`, which controls the individual Always Run flags for each split screen player.

## contents_t

### CONTENTS_PROJECTILECLIP (bit 14)

This new content flag will be collided against by `CONTENTS_PROJECTILE` entities.

### CONTENTS_PLAYER (bit 30)

This special content type flag is set only on player entities, and allows tracing to exclude/include them.

### CONTENTS_PROJECTILE (bit 31)

This special content type flag is set only on projectiles, and allows tracing to exclude/include them.

## surfflags_t

### SURF_ALPHATEST (bit 25)

This bit is widely supported by other engines and is supported in the rerelease.

### SURF_N64_UV (bit 28)

This flag is specific to N64, and halves texture sizes.

### SURF_N64_SCROLL_X (bit 29)

This flag is specific to N64, and causes textures to scroll in the X axis.

### SURF_N64_SCROLL_Y (bit 30)

This flag is specific to N64, and causes textures to scroll in the Y axis.

### SURF_N64_SCROLL_FLIP (bit 31)

This flag is specific to N64, and flips the scroll axis.

## csurface_t

This structure has undergone canonization of the 3.2x changes by Zoid.

### char[32] name

Uses the proper name length now.

### uint32_t id

This value must represent a unique ID that corresponds to its texinfo. The same ID must always reference the same texinfo, but they don't necessarily have to be sequential. Zero must always mean "no texinfo".

This is used by the client for indexing footstep sounds.

### char[16] material

The material ID for this texinfo, from the corresponding `.mat` file.

## trace_t

### csurface_t *surface

The only change is a contractual one: this value must never be null.

### cplane_t plane2 / csurface_t *surface2

When a trace impacts multiple planes at the destination, the collision system will now report both of them. The "second best" plane and surface are stored here. `surface2` *must* be null if a second surface was not hit.

This is used to solve some epsilon issues with the player movement system.

## cvar_t

### int32_t modified_count

The old `qboolean modified;` has been changed into an integral value. This value is increased when the cvar has been changed, but is **never** zero. The reason for this is so that "is cvar modified" checks always succeed on the first check, assuming you initialize the last modified value to 0.

The function `Cvar_WasModified` is provided as a convenience function to perform this task for you.

### int32_t integer

A common extension to Quake II, the integral value is stored at the end of the struct for you to use.

## player_state_t

### int32_t gunskin

This is a new value which sets the skin number used on the weapon.

### int32_t gunrate

This value sets the frame rate (in hz) of the players' weapon. For backwards compatibility, a value of 0 is equivalent to a value of 10. This ia mainly used for the Haste powerup, but in theory it could be used for future mods to have higher tickrate weapons in general.

### float[4] screen_blend / damage_blend

The full-screen `blend` value was split into two values: `screen_blend` and `damage_blend`.

`screen_blend` is the same as the original one, and is a full-screen color change. It is mainly used now for full-screen fades. To reduce the amount of screen flashing, the base game avoids flashing the screen whenever possible.

`damage_blend` is a new type of blend that occurs around the edge of the screen; this is used to replace many events that previously would flash the full screen.

### refdef_flags_t rdflags

The only adjustment to `rdflags` was the addition of a new flag: `RDF_NO_WEAPON_LERP`. This occupies bit 4, and can be used to temporarily disable interpolation on weapons.

### short[64] stats

`MAX_STATS` was increased from 32 to 64. Note that because stats are now handled by the game & cgame modules, you are not limited to a short for the purposes of packing down data/

### uint8_t team_id

For teamplay-oriented games, the player's team is sent in player state. While the client could derive this from entity state in theory, in practice that's a bit ugly since the players' entity may not even be visible (for instance if you've been gibbed), so this was the cleaner approach.

## usercmd_t

The fields `upmove`, `impulse` and `lightlevel` have been removed.

### button_t buttons

#### BUTTON_HOLSTER (bit 2)

This button corresponds to the new `+holster` command, which will keep the weapon holstered until depressed. It is used by the weapon wheel to allow the player to start switching weapons before the weapon wheel is dismissed.

#### BUTTON_JUMP (bit 3)
#### BUTTON_CROUCH (bit 4)

These two new bits replace `usercmd_t::upmove`, and determine the players' jumping and crouch states.

### vec3_t angles

These are now full float precision, allowing for players to aim more precisely.

### float forwardmove / sidemove

These are now full float, to allow controller inputs to be more precise. 

### uint32_t server_frame

New entry sent along with every usercmd, which tells the server which server frame that the input was depressed on. This is used for integrity checks, as well as for anti-lag hitscan.

## pmove_state_t

### pmtype_t pm_type

Two new pmove types have been added before `PM_SPECTATOR`, offsetting it and its subsequent entries by 2.

#### PM_GRAPPLE (1)

Used for the grappling hook; it informs client prediction that you should be pulled towards `velocity` and are not affected by gravity.

#### PM_NOCLIP (2)

This is what `PM_SPECTATOR` used to be, and prevents all clipping.

#### PM_SPECTATOR (3)

This value now represents spectator mode; you cannot enter walls, but can go through brush entities.

### vec3_t origin / vec3_t velocity / vec3_t delta_angles

These fields now have full float precision versus the original release. See [Pmove](#pmove) for more details.

### pmflags_t pm_flags

This type has had its capacity increased to `int16`. The following flags are new or adjusted:

#### PMF_NO_POSITIONAL_PREDICTION

This flag was originally called `PMF_NO_PREDICTION`; it now only disables prediction on origin, allowing angles to be predicted. **This is a backwards-incompatible change**, but should have very minimal impact on running old mods. This improves the feeling of the grappling hook.

#### PMF_ON_LADDER

This bit is used to signal back to the game that we are currently attached to a ladder.

#### PMF_NO_ANGULAR_PREDICTION

The angular equivalent of `PMF_NO_POSITIONAL_PREDICTION`.

#### PMF_IGNORE_PLAYER_COLLISION

This flag is input only, and tells Pmove to ignore `CONTENTS_PLAYER` contents.

#### PMF_TIME_TRICK

If set, then `pm_time` is the time remaining to start a trick jump.

### uint16_t pm_time

`pm_time` is now expressed in milliseconds instead of 8 * ms; since the code clamped subtractions on this to 1, it meant that high framerate players experienced slightly different physics, and in the case of trick jumps, had a smaller time gap to perform them.

### int8_t viewheight

A new field describing the viewheight output; this is for crouch prediction.

## pmove_t

The field `viewheight` has been removed, since it is now part of `pmove_state_t`.

### touch_list_t touch

The list of touched entities has been replaced with a list of traces, allowing the game to react better to touches.

### cplane_t groundplane

The plane that you're standing on is now returned by pmove.

### edict_t *player

An opaque handle to the player object, passed back to `trace`.

### trace / clip

`trace` is now sent the `passent` and `contentmask`, so it can perform more complex tracing routines.

`clip` is also now available to pmove, should you need it. It is currently only used in spectator movement, to clip solely against the world and nothing else.

### vec3_t viewoffset

The player's viewoffset is now passed in, to allow for accurate blending. Pmove is now semi-responsible for screen blends.

### vec3_t screen_blend

An output variable containing full-screen blends to apply to the view.

### refdef_flags_t rdflags

An output variable containing flags that should be merged with the server's representation.

### bool jump_sound

An output variable to tell the game to play a jumping sound.

### float impact_delta

When new ground is achieved, the impact is stored here for fall damage checks.

## edict_shared_t

NOTE: the following members of the old `edict_t` struct have been removed, and were moved server-side:
* `link_t area`
* `int num_clusters`
* `int clusternums[]`
* `int headnode`

### sv_entity_t sv

Most of the meat of the bot system is contained in the server code, and doesn't have direct access to the games' representation of the state of the game.

Bots use this thin interpretation of the game state data about entities to understand how to use entities to its advantage - similar to how the client receives a thin portion of entities to understand how to render them.

### bool linked

This boolean indicates whether the entity is currently linked into the world or not. It is the replacement of checking for `area.prev` being non-null.

### svflags_t svflags

For new functionality, some new flags were added to `svflags`. **This may cause backwards-incompatibility in older mods that have modified this enum!** This enum is server-specific, so it is always incorrect for mods to modify this.

#### SVF_PLAYER

This flag causes the object to be treated as `CONTENTS_PLAYER` for collision. All players have this flag.

#### SVF_BOT

This flag marks the entity as a bot.

#### SVF_NOBOTS

This flag tells the bot subsystem to ignore this entity.

#### SVF_RESPAWNING

This flag is a hint to the bot subsystem to inform it about how items respawn.

#### SVF_PROJECTILE

This flag treats the entity as `CONTENTS_PROJECTILE` for collision.

#### SVF_INSTANCED

This flag marks the entity as being instanced - it may be invisible or hidden for certain players.

#### SVF_DOOR

This flag is for the bot subsystem, and informs it that this entity is a door.

#### SVF_NOCULL

This flag overrides the client frame building culling routines, causing an entity to always be sent regardless of where it is (ignoring PVS/PHS, essentially). Its only use in our code is to keep no-attenuation looping speakers in frame always.

#### SVF_HULL

This flag adjusts the servers' method of clipping movement to entities. Normally, only `SOLID_BSP` entities will use their proper clipping bounds for collision, but if this is set on a `SOLID_TRIGGER` brush entity, traces will have to collide with the actual BSP tree of the trigger instead of solely touching its bounding box.

This is used in our game DLL to allow for certain triggers (like `trigger_gravity` or `trigger_flashlight`) to be activated when you are actually touching their brushes, allowing for angled triggers to finally exist.

## entity_state_t

### uint32_t number

Number was changed to `uint32_t` (from `int32_t`) to better represent its use and to only have to catch out of bounds in one direction.

### int32_t skinnum

Skinnum now packs a bit more data into it.

```cpp
// [Paril-KEX] player s.skinnum's encode additional data
union player_skinnum_t
{
    int32_t         skinnum;
    struct {
        uint8_t     client_num; // client index
        uint8_t     vwep_index; // vwep index
        int8_t      viewheight; // viewheight
        uint8_t     team_index : 4; // team #; note that teams are 1-indexed here, with 0 meaning no team
                                    // (spectators in CTF would be 0, for instance)
        uint8_t     poi_icon : 4;   // poi icon; 0 default friendly, 1 dead, others unused
    };
};
```

### effects_t effects

The type `effects_t` was changed from `uint32_t` to `uint64_t` since we have way more effects to express.

#### EF_BOB (bit 4)

Bit was unused in Quake II. This was repurposed into a weapon bobbing effect, similar to Quake III.

#### EF_POWERSCREEN (bit 9)

This effect uses a different model that is scaled to the monster's size now.

#### EF_DUALFIRE (bit 32)

This bit is used for a special effect, similar to `EF_QUAD`, but for Dualfire Damage.

#### EF_HOLOGRAM (bit 33)

This bit is used for the N64 hologram effect; it adds a spinning ball of green particles around the object.

#### EF_FLASHLIGHT (bit 34)

This bit marks a player entity as having a flashlight enabled. The effect itself is rendered separately by the client.

#### EF_BARREL_EXPLODING (bit 35)

This effect is used before an explobox explodes; it emits steam particles from the barrel, as if it is experiencing a decompression event.

#### EF_TELEPORTER2 (bit 36)

This effect is used for the teleporter FX in the N64.

#### EF_GRENADE_LIGHT (bit 37)

This effect creates a small light on monster grenades, to make them slightly easier to track visually.

### EF_FIREBALL (EF_ROCKET | EF_GIB)

This mutually-exclusive bit combo did nothing in the original game, since these special trails could only render one or the other. In the rerelease, it will render a fireball trail that begins yellow and large, tapering off into an orange trail, to mimick the effect on N64.

### renderfx_t renderfx

#### RF_NO_ORIGIN_LERP (bit 6)

This effect had a confusing name originally. Its name now reflects what it does: it disables origin interpolation.

#### RF_BEAM (bit 7)

You can now create custom segmented beams by setting a non-one modelindex on beams.

#### RF_CUSTOMSKIN (bit 8)

This effect was unused originally. It is now implemented and works as intended: specifying a `skinnum` will change the skin on the model to the skin specified in `CS_IMAGES + skinnum`. For `RF_FLARE`, `frame` must be used instead however, as `skinnum` is used for color data.

#### RF_NOSHADOW (bit 13)

This effect was client-only originally.

#### RF_CASTSHADOW (bit 14)

This effect marks an entity that casts light in the world; it is only used by `dynamic_light` (or dynamic `light` entities), and should not be used otherwise.

#### RF_SHELL_LITE_GREEN (bit 19)

This is the equivalent shell color for `EF_DUALFIRE`.

#### RF_CUSTOM_LIGHT (bit 20)

This flag creates a custom dynamic light at the position of the object. Its used in the N64 campaign, as it has custom light entities (`target_light`). `s.frame` is the light's radius, and `s.skinnum` is the light's current color (packed RGB).

#### RF_FLARE (bit 21)

This flag marks an entity as being rendered with a flare instead of the usual entity rendering. Flares overload some fields:
* `s.renderfx & RF_SHELL_RED` causes the flare to have an outer red rim.
* `s.renderfx & RF_SHELL_GREEN` causes the flare to have an outer green rim.
* `s.renderfx & RF_SHELL_BLUE` causes the flare to have an outer blue rim.
* `s.renderfx & RF_FLARE_LOCK_ANGLE` causes the flare to not rotate towards the viewer.
* `s.renderfx & RF_CUSTOMSKIN` causes the flare to use the custom image index in `s.frame`.
* `s.modelindex2` is the start distance of fading the flare out.
* `s.modelindex3` is the end distance of fading the flare out.
* `s.skinnum` is the RGBA of the flare.

#### RF_OLD_FRAME_LERP (bit 22)

This flag signals that `s.old_frame` should be used for the next frame and respected by the client. This can be used for custom frame interpolation; its use in this engine is specific to fixing interpolation bugs on certain monster animations.

#### RF_DOT_SHADOW (bit 23)

Draw a blob shadow underneath the entity.

#### RF_LOW_PRIORITY (bit 24)

This flag marks an entity as low priority; if the renderer runs out of entity slots, these entities will be eligible for replacement. For instance, a monster is more important than a gib, so gibs are marked low priority so they can be replaced by a monster if the limit is reached.

#### RF_NO_LOD (bit 25)

The original MD2 models will be used for LOD. Setting this bit prevents this behavior.

#### RF_NO_STEREO (bit 2)

This is an overloaded flag that only applies to non-rendered entities that contain sounds. If set, stereo panning is disabled on this entity.

#### RF_STAIR_STEP (bit 26)

The tick rate increase caused a bit of a visual bug with monsters and players: they now stepped up steps within 0.025 seconds instead of 0.1, causing jarring hitching. To fix this, entities set this flag when they detect they have stepped up a stair, and the client will interpolate their height difference over 0.1 seconds.

#### RF_BEAM_LIGHTNING (RF_BEAM | RF_GLOW)

This mutually-exclusive bit combo causes a laser to become a lightning bolt, for N64 support.

### uint32_t solid

This was changed from `int32_t` to `uint32_t`, and now packs more data into it to better represent bounding boxes to clients. 

For backwards compatibility, 31 is still the magic value used for BSP entities. The actual packed data, however, is now as follows:

```cpp
union solid_packed_t
{
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t zd; // always negative
        uint8_t zu; // encoded as + 32
    } p;

    uint32_t u;
};

// packing:

packed.p.x = ent->maxs[0];
packed.p.y = ent->maxs[1];
packed.p.zd = -ent->mins[2];
packed.p.zu = ent->maxs[2] + 32;

// unpacking:
packed.u = state->solid;
ent->mins[0] = -packed.p.x;  ent->maxs[0] = packed.p.x;
ent->mins[1] = -packed.p.y;  ent->maxs[1] = packed.p.y;
ent->mins[2] = -packed.p.zd; ent->maxs[2] = packed.p.zu - 32;
```

This is similar to Quake III Arena, and essentially allows any integral bbox to make it to the clients unchanged.

### entity_event_t event

Two new events were added:

#### EV_OTHER_FOOTSTEP (8)

Allows non-players to send footsteps. They have idle attenuation, whereas regular footsteps have normal attenuation.

#### EV_LADDER_STEP (9)

Ladder climbing 'footstep' event.

### float alpha

This value allows you to specify exact transparency values for entities. For backwards compatibility, setting it to zero should be equivalent to an unchanged value, but any non-zero value should be respected as changed.

### float scale

This value allows you to scale an entity by the given amount. For backwards compatibility, setting it to zero should be equivalent to an unchanged value, but any non-zero value should be respected as changed.

### uint8_t instance_bits

This value is not meant to be set directly by the game code, but will have non-zero bits set for split-screen players that cannot see this entity.

### float loop_volume / loop_attenuation

Looping noises can now have volume and attenuation explicitly specified. For both, a value of zero indicates default/unchanged, for backwards compatibility. For `loop_attenuation`, a value of `-1` indicates full level audio (like `ATTN_NONE`).

### int32_t owner

An entity's owner is now networked, allowing for it to ignore collision properly.

### int32_t old_frame

Only sent when `renderfx & RF_OLD_FRAME_LERP` - indicates that this frame is the frame to lerp from.

# Import/Exports

## Game Import

### (read-only) tick_rate / frame_time_s / frame_time_ms

This holds the server's tick variables. They will be set at the start of the server, before [PreInit](#preinit).
`tick_rate` stores the tick rate, in hz.
`frame_time_s` is the time a game frame will take in seconds.
`frame_time_ms` is the time a game frame will take in ms.

These are provided pre-calculated for convenience.

### Broadcast_Print

This function writes `message` with the print type of `printlevel` to all players. See [Print Adjustments](#print-adjustments). This is kept for compatibility purposes, [Loc_Print](#loc_print) replaces it.

### Com_Print

This function writes `message` to the server. See [Print Adjustments](#print-adjustments).

### Client_Print

This function writes out `message` with the print type of `printlevel` to the specified `ent` player. See [Print Adjustments](#print-adjustments). This is kept for compatibility purposes, [Loc_Print](#loc_print) replaces it.

### Center_Print

This function writes `message` to the specified `ent` player in the center of their screen. See [Print Adjustments](#print-adjustments). This is kept for compatibility purposes, [Loc_Print](#loc_print) replaces it.

### sound / positioned_sound

The `channel` enum has a single new flag:

#### CHAN_FORCE_POS (bit 5)

If set (and an origin is **not** supplied), the entity's origin will be forced to be used as the origin point of the sound even if there is a better position available.

### local_sound

This function was introduced to deal with some split-screen issues that popped up. It's designed to mimick `localsound` of QuakeC; it will directly send a sound packet to the specified player, using a `dupe_key` if supplied (see [unicast](#unicast)).

See [sound](#sound--positioned_sound) for info about the channels.

### get_configstring

This function fetches a configstring from the servers' current configstring data.

### Com_Error

See [Print Adjustments](#print-adjustments).

### clip

This is a new function designed to fit a specific purpose: it will test if the box specified by `mins` & `maxs`, moved from `start` to `end`, will clip against the specified `entity` with the given `contentmask`. As an example, you could use this to detect if an entity is actually intersecting a brush in a trigger instead of just being within its bounding box.

### inPVS / inPHS

This function now accepts a boolean, `portals`, which changes whether or not it should ignore areaportals.

### BoxEdicts

This function was modified with a simple filtering callback, greatly extending its purpose and removing some limitations that would occur with previous uses. The filter callback is called for every entity discovered, and you can choose to include or skip entities that it finds, or even completely abort the search. In addition, you can now call the function with a 0 `maxcount`, and the function will still continue to filter and find entities, reporting the final count. To match the old behavior, if a non-zero `maxcount` is supplied, the return count will cap out at `maxcount`.

Note that it is disallowed to modify world links (linkentity/unlinkentity, etc) in a filter callback, it can only be used for filtering.

### unicast

The `dupe_key` parameter is new, and is to solve a very peculiar issue with split screen players. When unicast is used to spawn effects or sounds, it may not be desirable to replay the same effect on multiple split screens, since split screen players are all the same client and share views. For example, if you do a unicast for a `TE_BLASTER` somewhere in the world for every player, for a split screen client with 4 players, that effect will play 4 times - even though all four players are viewing the same world. The game DLL also has no knowledge or understanding of split screen, so there's no way for the game to work around it.

Instead of having the game need to know this kind of implementation detail and prevent double-sending, for effects that are going to potentially be sent to multiple players that *may* be on a split screen, you can specify a dupe key value. This value, when non-zero, will be marked as "already sent" for that client, and won't be sent again for the next packet if it was already tripped. The game DLL provides the `GetUnicastKey` global which will give you a rolling value to directly pass into unicast or local_sound.

### WriteFloat

Implemented; this was stubbed out of the old code.

### WritePosition

Now sends full float positions.

### WriteDir / WriteAngle

Unchanged - WriteAngle is for compressed angles, when high precision is not necessary.

### WriteEntity

New function to write an entity, to make it easier to write them without needing to WriteShort directly.

### GetExtension

See [Extensions](#extensions).

### Bot_RegisterEdict

Informs the bot subsystem that an entity needs to be registered.

### Bot_UnRegisterEdict

Informs the bot subsystem that an entity needs to be unregistered.

### Bot_MoveToPoint

Forces a bot to move to the specified point.

### Bot_FollowActor

Forces a bot to follow the specified actor.

### GetPathToGoal

The main pathfinding function; with the given pathfinding `request`, you'll be given `info` about the operation, the path, etc.

### Loc_Print

The new primary entry point for printing. This function replaces all of the others (except Com_Print).
For basic usage, it can be called on an entity (or nullptr for broadcasting) with the correct `level`, with the message to send in `base`, and nullptr `args` along with 0 `num_args`. For actual localized messages, however, you can send additional arguments via the `args`/`num_args` parameters which are sent to the client for further processing.

In addition to localization, `level` now has new values and bit flags.

#### PRINT_TYPEWRITER (4)

Causes the message to be printed out one at a time, like a typewriter. Used for objectives, similar to the N64 version.

#### PRINT_CENTER (5)

An instant centerprint, like the legacy centerprints.

#### PRINT_TTS (6)

Identical to `PRINT_HIGH` in importance, but additionally causes text to speech narration to activate if enabled on the client.

#### PRINT_BROADCAST (bit 3)

Message will be sent to all players.

#### PRINT_NO_NOTIFY (bit 4)

Message will not be sent to the notify system.

### Draw_Line / Draw_Point / Draw_Circle / Draw_Bounds / Draw_Sphere / Draw_OrientedWorldText / Draw_StaticWorldText / Draw_Cylinder / Draw_Ray

These functions are debugging aids that only render on the server.

### ReportMatchDetails_Multicast

This function is solely for platforms that need match result data.

### ServerFrame

Returns the server's frame number.

### SendToClipBoard

Copy data to the server's clipboard, useful for debugging.

### Info_ValueForKey / Info_RemoveKey / Info_SetValueForKey

See [Info Keys](#info-keys).

## Save Games

One of the major changes to this release of Quake II is the save system. Instead of storing pointer offsets and copies of memory, the level & game data is written to UTF-8 JSON. This makes save data much easier to navigate for a human & developer that wants to look into a bug, while also being quick and efficient for storage.

The save system, as a result, no longer interfaces with the filesystem at all. Other mods are not required to use JSON, any text format will work as the server and client do not interact with the data.

## Game Export

### (read-only) apiversion

The version # reported by the server.

### PreInit

This function is called before InitGame, and should be where you initialize your mod's latched cvars. This can be used to fix any conflicting latched cvars, which will be "locked in" after this is called.

### SpawnEntities

All three parameters are now properly marked const.

### WriteGameJson

See [Save Games](#save-games).

### ReadGameJson

See [Save Games](#save-games).

### WriteLevelJson

This function is now informed whether the level write is from a level transition or a manual save.
See [Save Games](#save-games).

### ReadLevelJson

See [Save Games](#save-games).

### CanSave

This new export now dictates whether the game is saveable or not.

### ClientChooseSlot

ClientChooseSlot is intended to take in a bunch of information about the client that is connecting, and choose which `edict_t` entity this player should occupy. It is used in the rerelease to reorder players consistently throughout coop games, and ensure that everybody always gets the correct slot.

Callers are given the player's `userinfo` and `social_id` (the social ID is a unique value per player on certain platforms), which you can use to find the correct slot from the current saved client data. You're also told whether the client `isBot`, which should always use non-saved available slots first. The `ignore` field will give you a list of slots up to `num_ignore` entities that are already occupied or were reported as such, so they can be safely skipped over. Finally, the `cinematic` parameters will tell you whether the loaded map is a video, which in most cases reordering will not be necessary.

### ClientConnect

The function is now given the `social_id` and `isBot` state of the connecting client.

### RunFrame

This function now receives a boolean to tell whether the call is from the main game loop, or from some other source (the game is settling, or running frames to advance level transitions). If the latter is occurring, you can use this boolean to speed up level transitions by skipping logic that is not necessary but is CPU-intensive, such as enemies searching for players to attack.

### PrepFrame

This function used to be in the server, but is now controlled by the game DLL. It's ran after the game has execute a frame & has sent the packet data over to all players. Things like hit markers and one-shot events are cleared in here.

### edict_size / num_edicts / max_edicts

These were changed to size_t and uint32_t/uint32_t respectively, to better represent their use.

### server_flags

This is an integer shared between server and game, which stores bits for special states that the server cares about.

### Pmove

See [Pmove](#pmove).

### GetExtension

See [Extensions](#extensions).

### Bot_SetWeapon

Called by the bot subsystem to switch weapons.

### Bot_TriggerEdict

Called by the bot subsystem to trigger an entity.

### Bot_UseItem

Called by the bot subsystem to use an item.

### Bot_GetItemID

Fetch an item ID by a classname; for the bot subsystem.

### Entity_IsVisibleToPlayer

This function is for item instancing; the rerelease of Quake II supports instanced items, which will display only for the players who haven't picked it up yet. For online players, this simply removes the item if you've gotten it, but for split screen players it will show a ghost where the entity was on players that have already picked it up.

### GetShadowLightData

This function fetches data for the given shadow light for building client frames.

## Player State

In the original client, player state was often accessed directly to perform various tasks or render things. Much of this has been moved into the cgame module to allow increased customization.

## Client Game Import

### (read-only) tick_rate
### (read-only) frame_time_s
### (read-only) frame_time_ms

This holds the server's tick variables. They will be set at the start of the client, before Init.
`tick_rate` stores the tick rate, in hz.
`frame_time_s` is the time a game frame will take in seconds.
`frame_time_ms` is the time a game frame will take in ms.

These are provided pre-calculated for convenience.

### Com_Print

Print a debug message to the client.

### get_configstring

Fetch the given configstring data from the client.

### Com_Error

Abort error for client.

### TagMalloc / TagFree / FreeTags

Same as server.

### cvar / cvar_set / cvar_forceset

Same as server.

### AddCommandString

Push command(s) into the command buffer on the client side.

### GetExtension

See [Extensions](#extensions).

### CL_FrameValid

Returns true if the current frame being rendered is valid.

### CL_FrameTime

Returns the current frame time delta.

### CL_ClientTime

Returns the client's current time (server-bound).

### CL_ClientRealTime

Returns the client's current real, unbound time.

### CL_ServerFrame

Returns the client's server frame.

### CL_ServerProtocol

Returns the client's connected server protocol.

### CL_GetClientName

Returns a UTF-8 string containing the givern player's name.

### CL_GetClientPic

Returns a string containing the given player's icon.

### CL_GetClientDogtag

Returns a string containing the given player's dogtag.

### CL_GetKeyBinding

Returns a key binding for the given key. Returns an empty string if the key is unbound.

### Draw_RegisterPic

Precache the given image.

### Draw_GetPicSize

Returns the size of the given image.

### SCR_DrawChar

Draw the given conchars char at the specified position. A `shadow` parameter has been added to draw a drop shadow.

### SCR_DrawPic

Draw the given pic at the specified position.

### SCR_DrawColorPic

Draw the given pic at the specified location, with the specified color.

### SCR_SetAltTypeface

Change whether the alternate (accessibility) typeface is in use or not.

### SCR_DrawFontString

Draw a string to the screen, using the Kex KFONT which includes non-latin characters.

### SCR_MeasureFontString

Measure the size of the string as it would be rendered.

### SCR_FontLineHeight

Returns the line height of the font.

### CL_GetTextInput

Returns a pointer to the current text input, and whether this input is for team say or not.

### CL_GetWarnAmmoCount

For the given weapon ID, get the amount that is considered to be low ammo.

### Localize

Localize the given string and arguments to an output buffer.

### SCR_DrawBind

Draw a user bind to the screen, returns the Y offset from rendering.

### CL_InAutoDemoLoop

Returns true if the engine is running the attract demo loop.

## Client Game Export

### (read-only) apiversion

API version.

### Init / Shutdown

Lifecycle functions for the client game. Note that the cgame does not control UI, so the cgame only exists when you are connected and in-game.

### DrawHUD

This function is called by the client when their HUD needs to be rendered.
- `isplit` contains the split screen index of the player.
- `data` contains a pointer to some transient information from the server. This includes currently active layout, and the player's active inventory when the inventory is open.
- `hud_vrect` contains the unpadded rectangle of the HUD being rendered.
- `hud_safe` contains the size of the safe area. Only x and y are set, w and h are unused.
- `scale` is the integral scale of the HUD being rendered.
- `playernum` is the player's client index.
- `ps` is a pointer to the player's current player state.

### TouchPics

Function called for precaching images used by the HUD.

### LayoutFlags

For the given player state, return the `layout_flags_t` that would match it.

### GetActiveWeaponWheelWeapon / GetOwnedWeaponWheelWeapons / GetWeaponWheelAmmoCount / GetPowerupWheelCount

The weapon wheel is in the client, but uses these callbacks to fetch data from `player_state_t`.

### GetHitMarkerDamage

Returns how much damage was done for this player.

### Pmove

See [Pmove](#pmove).

### ParseConfigString

When a configstring is received, the cgame is also notified of changes. The cgame module can react to configstring updates here.

### ParseCenterPrint

When a centerprint-like message is received by the client, it is sent to the cgame via this function.
- `isplit` is the split screen player it was sent to.
- `instant` is true if the message is a centerprint that is drawn without the typewriter effect.

### ClearNotify

The client will call this when the notification area should be cleared.

### ClearCenterprint

The client will call this when centerprints should be cleared.

### NotifyMessage

When a notify message is received, the client will send it to this function.
- `isplit` is the split screen player it was sent to.
- `is_chat` is true if it was a chat-like message.

### GetMonsterFlashOffset

To simplify the server to client muzzleflash communication, the cgame now exports muzzleflash origins via this function.

### GetExtension

See [Extensions](#extensions).

# Quake II server protocol - version 2023

The Quake II rerelease features an updated server protocol. Most of the messages are backwards compatible, but some needed adjustments to work with new or changed features, or raised limits.

This document will only outline the changes since the original release, rather than the whole protocol.

## (out of band, client <-> server) challenges

The out of band challenges have been removed.

## (out of band, client -> server) connect

The `connect` message is similar to the original, but has redundant information removed. Port and challenge are handled at a lower level, so that information is not included. The `connect` message is in the following format:

`connect {protocol} {num split} {socials...} {userinfo...}`

- `protocol` is 2023
- `num split` is the number of split screen players
- `socials` is `num split` number of arguments containing each players' social identifiers
- `userinfo` is the clients' userinfo string, split up by groups of 510 characters each (since command arguments have a maximum length). This can often span 2 or more arguments, since each userinfo var has a value per player. See [Info Keys](#info-keys).

## (out of band, server -> client) client_connect

This message is sent when the server accepts the connection. It is in the following format:

`client_connect {protocol}`

- `protocol` is 2023

The protocol version is sent mainly for backward compatibility with demos.

## (packet, server -> client) svc_muzzleflash (1)

The following enum values are now accepted.

### MZ_BFG2 (19)

Secondary muzzleflash for the BFG, sent when the BFG actually fires.

### MZ_PHALANX2 (20)

Secondary muzzleflash for the Phalanx, sent for the second projectile.

### MZ_PROX (31)

Sent when the Prox Launcher is fired.

### MZ_ETF_RIFLE_2 (32)

Sent when the other barrel of the ETF Rifle is fired.

## (packet, server -> client) svc_muzzleflash2 (2)

### MZ2_BOSS2_MACHINEGUN_L2 / MZ2_BOSS2_MACHINEGUN_R2 (74 / 134)

These two values were just copies of L1/R1, but were repurposed to make Hyperblaster-specific sounds for the new Hornet.

The following enum values are now accepted.

### MZ2_SOLDIER_RIPPER_1 - MZ2_SOLDIER_HYPERGUN_8 (211 - 226)

Muzzleflashes for the ripper & blue hyperblaster guards.

### MZ2_GUARDIAN_BLASTER - MZ2_ARACHNID_RAIL_UP2 (227 - 231)

Muzzleflashes for the PSX monsters.

### MZ2_INFANTRY_MACHINEGUN_14 - MZ2_INFANTRY_MACHINEGUN_21 (232 - 239)

Muzzleflashes for the Infantry's run-attack animation.

### MZ2_GUNCMDR_CHAINGUN_1 - MZ2_GUNCMDR_GRENADE_CROUCH_3 (240 - 250)

Muzzleflashes for the Gunner Commander.

### MZ2_SOLDIER_BLASTER_9 - MZ2_SOLDIER_HYPERGUN_9 (251 - 255)

Muzzleflashes for the guards' new prone-firing animation.

## (packet, server -> client) svc_muzzleflash3 (32)

This packet was necessitated from running out of bits in svc_muzzleflash2. The only difference is the byte for `id` is a ushort.

### MZ2_GUNNER_GRENADE2_1 - MZ2_GUNNER_GRENADE2_4 (256 - 259)

Alternate firing animation for the Gunner's grenade launcher.

### MZ2_INFANTRY_MACHINEGUN_22 (260)

Alternate firing animation for the Infantry.

### MZ2_SUPERTANK_GRENADE_1 (261 - 262)

Supertank's grenade launcher.

### MZ2_HOVER_BLASTER_2 / MZ2_DAEDALUS_BLASTER_2 (263 / 264)

The Icarus and Daedalus' opposite side blaster.

### MZ2_MEDIC_HYPERBLASTER1_1 - MZ2_MEDIC_HYPERBLASTER1_12 / MZ2_MEDIC_HYPERBLASTER2_1 - MZ2_MEDIC_HYPERBLASTER2_12 (265 - 276 / 277 - 288)

The Medic and Medic Commander's Hyperblaster firing animation sweep.

## (packet, server -> client) svc_temp_entity (3)

As documented in [WritePosition](#writeposition), WritePos now writes full float precision, so ReadPos has to read full float.

### TE_SPLASH (10)

The "color/splash" enumeration accepts a new value:

#### SPLASH_ELECTRIC (7)

A spark used exclusively in N64, which spawns blue/white particles and makes sparking noises.

The following new enum values are accepted:

### TE_RAILTRAIL2 (31)

This effect was unused in Quake II, and was retooled to a lighter railgun effect used for Instagib mode.

### TE_BLUEHYPERBLASTER (56)
"Correct" version of the old buggy `TE_BLUEHYPERBLASTER`, which is now `TE_BLUEHYPERBLASTER_DUMMY`.
- ReadPos
- ReadDir

### TE_BFG_ZAP (57)
Laser when an entity has been zapped by a BFG explosion.
- ReadPos (start)
- ReadPos (end)

### TE_BERSERK_SLAM (58)
Large blue flash & particles at impact point towards a direction.
- ReadPos
- ReadDir

### TE_GRAPPLE_CABLE_2 (59)
The grappling hook in Quake II 3.20 used a larger message that didn't allow the cable to render like other player-derived beams.
- ReadEntity
- ReadPos (start)
- ReadPos (end)

### TE_POWER_SPLASH (60)
Effect sent when a power shield evaporates.
- ReadEntity
- ReadByte (1 for screen, 0 for armor)

### TE_LIGHTNING_BEAM (61)
A lightning bolt that originates from the player, like the heat beam. Unused.
- ReadEntity
- ReadPos (start)
- ReadPos (end)

### TE_EXPLOSION1_NL / TE_EXPLOSION2_NL (62 / 63)
Variants of explosion that don't include any dynamic light.
- ReadPos

## (packet, server -> client) svc_sound (9)

Since `MAX_EDICTS` is now 8192, this packet required changes to support higher entity numbers. `MAX_SOUNDS` being increased to 1024 also necessitated the sound index changing from byte to ushort.

- ReadByte (flags)
- ReadShort (soundindex)
- [if flags & SND_VOLUME] ReadByte (volume)
- [if flags & SND_ATTENUATION] ReadByte (attenuation)
- [if flags & SND_OFFSET] ReadByte (offset)
- [if flags & SND_ENT]
  - [if flags & SND_LARGE_ENT] ReadLong (entchan)
  - [if !(flags & SND_LARGE_ENT)] ReadShort (entchan)
- [if flags & SND_POS] ReadPos (origin)

`entchan` is encoded as such:
```
struct sndchan_t
{
	uint8_t		channel : 3;
	uint32_t	entity : 29;
}
```

`flags` contains the following bits:
- SND_VOLUME (bit 0)
- SND_ATTENUATION (bit 1)
- SND_POS (bit 2)
- SND_ENT (bit 3)
- SND_OFFSET (bit 4)
- SND_EXPLICIT_POS (bit 5)
- SND_LARGE_ENT (bit 6)

Note that `SND_POS` is **always** set. This is to fix a legacy bug where sounds played on entities outside of your PVS will play at the origin instead of their real location. The client should pick the real position if the entity is in their frame, but otherwise fall back to the sound packets' position.

## (packet, server -> client) svc_print (10)

This packet now supports `PRINT_TYPEWRITER` and `PRINT_CENTER` values. See [Loc_Print](#loc_print).

## (packet, server -> client) svc_stufftext (11)

For security reasons, this packet will only allow commands things to be executed.

## (packet, server -> client) svc_serverdata (12)

- ReadLong (protocol)
- ReadLong (spawncount)
- ReadByte (0 = game, 1 = demo, 2 = server demo)
- ReadByte (tickrate)
- ReadString (gamedir)
- ReadShort[N] (playernums; see below)
- ReadString (level name)

To parse `playernums`, read the first short and check its value. If it is -2, then read an additional short, which is the number of split screen entities to follow. Read that number of shorts to get each entity number for each split screen player. Otherwise, the value returned by the initial ReadShort is the playernum of the client.

The special value -1 will be used in cinematics, to indicate that the player has no entity.

## (packet, server -> client) svc_frame (20)

- ReadLong (serverframe)
- ReadLong (deltaframe)
- ReadByte (surpressCount)

For each player in this client's `numSplit` the following data is parsed:

- ReadByte (areabits length)
- ReadData (using above byte)
- ReadByte (value will be `svc_playerinfo`)
- ParsePlayerState (see [svc_playerinfo](#svc_playerinfo-17))

Then, back to `svc_frame` data:

- client entity `event`s should all be cleared back to `EV_NONE`
- ReadByte (value will be `svc_packetentities`)
- ParsePacketEntities (see [svc_packetentities](#svc_packetentities-18))

### svc_playerinfo (17)

#### Bits

```c
#define PS_M_TYPE           (1<<0)
#define PS_M_ORIGIN         (1<<1)
#define PS_M_VELOCITY       (1<<2)
#define PS_M_TIME           (1<<3)
#define PS_M_FLAGS          (1<<4)
#define PS_M_GRAVITY        (1<<5)
#define PS_M_DELTA_ANGLES   (1<<6)

#define PS_VIEWOFFSET       (1<<7)
#define PS_VIEWANGLES       (1<<8)
#define PS_KICKANGLES       (1<<9)
#define PS_BLEND            (1<<10)
#define PS_FOV              (1<<11)
#define PS_WEAPONINDEX      (1<<12)
#define PS_WEAPONFRAME      (1<<13)
#define PS_RDFLAGS          (1<<14)

#define PS_MOREBITS         (1<<15)

// [Paril-KEX]
#define PS_DAMAGE_BLEND     (1<<16)
#define PS_TEAM_ID          (1<<17)
```

#### Data

- ReadUShort (flags)
- [if flags & PS_MOREBITS] flags |= ReadUShort \<\< 16
- [if flags & PS_M_TYPE] ReadByte (pm_type)
- [if flags & PS_M_ORIGIN] ReadPos (pm_origin)
- [if flags & PS_M_VELOCITY] ReadPos (pm_velocity)
- [if flags & PS_M_TIME] ReadUShort (pm_time)
- [if flags & PS_M_FLAGS] ReadUShort (pm_flags)
- [if flags & PS_M_GRAVITY] ReadShort (pm_gravity)
- [if flags & PS_M_DELTA_ANGLES] ReadPos (pm_delta_angles)
- [if flags & PS_VIEWOFFSET]:
```cpp
	viewoffset_x = ReadShort() * (1.f / 16.f)
	viewoffset_y = ReadShort() * (1.f / 16.f)
	viewoffset_z = ReadShort() * (1.f / 16.f)
	viewheight = ReadChar() // note: not in protocol 2022
```
- [if flags & PS_VIEWANGLES] ReadPos (viewangles)
- [if flags & PS_KICKANGLES]
```cpp
kick_angles_x = ReadShort() / 1024.f
kick_angles_y = ReadShort() / 1024.f
kick_angles_z = ReadShort() / 1024.f
```
- [if flags & PS_WEAPONINDEX]
```cpp
gunindex_temp = ReadUShort()
gunskin = (gunindex_temp & 0xE000) >> 13
gunindex = gunindex_temp & ~0xE000
```
- [if flags & PS_WEAPONFRAME]
```cpp
#define GUNBIT_OFFSET_X (1<<0)
#define GUNBIT_OFFSET_Y (1<<1)
#define GUNBIT_OFFSET_Z (1<<2)
#define GUNBIT_ANGLES_X (1<<3)
#define GUNBIT_ANGLES_Y (1<<4)
#define GUNBIT_ANGLES_Z (1<<5)
#define GUNBIT_GUNRATE (1<<6)
```
```cpp
gunframe_temp = ReadUShort()
gun_bits = (gunframe_temp & 0xFE00) >> 9
gunframe = (gunframe_temp & ~0xFE00)

[if gun_bits & GUNBIT_OFFSET_X] gunoffset_x = ReadFloat()
[if gun_bits & GUNBIT_OFFSET_Y] gunoffset_y = ReadFloat()
[if gun_bits & GUNBIT_OFFSET_Z] gunoffset_z = ReadFloat()
[if gun_bits & GUNBIT_ANGLES_X] gunangles_x = ReadFloat()
[if gun_bits & GUNBIT_ANGLES_Y] gunangles_y = ReadFloat()
[if gun_bits & GUNBIT_ANGLES_Z] gunangles_z = ReadFloat()
[if gun_bits & GUNBIT_GUNRATE] gunrate = ReadByte()
```
- [if flags & PS_BLEND]
```cpp
screen_blend_r = ReadByte() / 255.f
screen_blend_g = ReadByte() / 255.f
screen_blend_b = ReadByte() / 255.f
screen_blend_a = ReadByte() / 255.f
```
- [if flags & PS_FOV] ReadByte(fov)
- [if flags & PS_RDFLAGS] ReadByte(rdflags)
- ReadLong(statbits)
```cpp
for (i = 0; i < 32; i++)
	if (statbits & (1 << i))
		ReadShort(stats[i])
```
- ReadLong(morestatbits)
```cpp
for (i = 32; i < 64; i++)
	if (morestatbits & (1 << (i - 32)))
		ReadShort(stats[i])
```
- [if flags & PS_DAMAGE_BLEND]
```cpp
damage_blend_r = ReadByte() / 255.f
damage_blend_g = ReadByte() / 255.f
damage_blend_b = ReadByte() / 255.f
damage_blend_a = ReadByte() / 255.f
```
- [if flags & PS_TEAM_ID]
```cpp
team_id = ReadByte()
```

### svc_packetentities (18)

#### Bits
```c

// try to pack the common update flags into the first byte
#define U_ORIGIN1   (1<<0)
#define U_ORIGIN2   (1<<1)
#define U_ANGLE2    (1<<2)
#define U_ANGLE3    (1<<3)
#define U_FRAME8    (1<<4)      // frame is a byte
#define U_EVENT     (1<<5)
#define U_REMOVE    (1<<6)      // REMOVE this entity, don't add it
#define U_MOREBITS1 (1<<7)      // read one additional byte

// second byte
#define U_NUMBER16  (1<<8)      // NUMBER8 is implicit if not set
#define U_ORIGIN3   (1<<9)
#define U_ANGLE1    (1<<10)
#define U_MODEL     (1<<11)
#define U_RENDERFX8 (1<<12)     // fullbright, etc
#define U_EFFECTS8  (1<<14)     // autorotate, trails, etc
#define U_MOREBITS2 (1<<15)     // read one additional byte

// third byte
#define U_SKIN8     (1<<16)
#define U_FRAME16   (1<<17)     // frame is a short
#define U_RENDERFX16 (1<<18)    // 8 + 16 = 32
#define U_EFFECTS16 (1<<19)     // 8 + 16 = 32
#define U_MODEL2    (1<<20)     // weapons, flags, etc
#define U_MODEL3    (1<<21)
#define U_MODEL4    (1<<22)
#define U_MOREBITS3 (1<<23)     // read one additional byte

// fourth byte
#define U_OLDORIGIN (1<<24)     // FIXME: get rid of this
#define U_SKIN16    (1<<25)
#define U_SOUND     (1<<26)
#define U_SOLID     (1<<27)
#define U_MODEL16   (1<<28)
#define U_EFFECTS64 (1<<29) // [Edward-KEX]
#define U_ALPHA     (1<<30) // [Paril-KEX]
#define U_MOREBITS4 (1<<31) // [Paril-KEX] read one additional byte
#define U_SCALE     (1ull<<32ull) // [Paril-KEX]
#define U_INSTANCE  (1ull<<33ull) // [Paril-KEX]
#define U_OWNER     (1ull<<34ull) // [Paril-KEX]
#define U_OLDFRAME  (1ull<<35ull) // [Paril-KEX]
```

#### Data

The regular process for deltaing entities has not changed, but the data bits have.

ParseEntityBits:
- ReadByte(bits)
- [if bits & U_MOREBITS1] bits |= ReadByte() \<\< 8
- [if bits & U_MOREBITS2] bits |= ReadByte() \<\< 16
- [if bits & U_MOREBITS3] bits |= ReadByte() \<\< 24
- [if bits & U_MOREBITS4] bits |= ReadByte() \<\< 32
- [if bits & U_NUMBER16] ReadShort(number) [else] ReadByte(number)

ParseDelta:
- [if bits & U_MODEL16]
```cpp
ReadShort(modelindex)
ReadShort(modelindex2)
ReadShort(modelindex3)
ReadShort(modelindex4)
```
- [else]
```cpp
ReadByte(modelindex)
ReadByte(modelindex2)
ReadByte(modelindex3)
ReadByte(modelindex4)
```
- [if bits & U_FRAME8] ReadByte(frame)
- [if bits & U_FRAME16] ReadShort(frame)
- [if bits & (U_SKIN8 | U_SKIN16) == (U_SKIN8 | U_SKIN16)] ReadLong(skinnum)
- [elseif bits & U_SKIN8] ReadByte(skinnum)
- [elseif bits & U_SKIN16] ReadUShort(skinnum)
- [if bits & (U_EFFECTS8 | U_EFFECTS16 | U_EFFECTS64)]
```cpp
// if 64-bit effects are sent, the low bits are sent first
// and the high bits come after.
[if bits & U_EFFECTS64] ReadULong(loeffects)

[if bits & (U_EFFECTS8 | U_EFFECTS16) == (U_EFFECTS8 | U_EFFECTS16)] ReadULong(effects)
[elseif bits & U_EFFECTS16] ReadUShort(effects)
[else] ReadByte(effects)

[if bits & U_EFFECTS64] effects = (effects << 32) | loeffects 
```
- [if bits & (U_RENDERFX8 | U_RENDERFX16) == (U_RENDERFX8 | U_RENDERFX16)] ReadLong(effects)
- [elseif bits & renderfx] ReadByte(renderfx)
- [elseif bits & U_RENDERFX16] ReadShort(renderfx)
- [if bits & U_SOLID] ReadULong(solid)
```cpp
// note: for the protocol in the demos (2022), if `solid` is zero,
// then the following reads are lower precision, using ReadShort() * (1.f / 8.f)
[if bits & U_ORIGIN1] ReadFloat(origin_x)
[if bits & U_ORIGIN2] ReadFloat(origin_y)
[if bits & U_ORIGIN3] ReadFloat(origin_z)
[if bits & U_OLDORIGIN] ReadPos(oldorigin)
```
- [if bits & U_ANGLE1] ReadFloat(angle_x)
- [if bits & U_ANGLE2] ReadFloat(angle_y)
- [if bits & U_ANGLE3] ReadFloat(angle_z)
- [if bits & U_SOUND] ReadUShort(temp_sound)
```cpp
bool has_volume = temp_sound & 0x4000
bool has_attenuation = temp_sound & 0x8000;

// the sound index takes up 14 bits
sound = temp_sound & ~(0x4000 | 0x8000)

[if has_volume] loop_volume = ReadByte() / 255.f
[else] loop_volume = 1.f

[if has_attn] loop_attenuation = ReadByte()
[else] loop_attenuation = ATTN_STATIC
```
- [if bits & U_EVENT] ReadByte(event) [else] event = 0
- [if bits & U_ALPHA] alpha = ReadByte() / 255.f
- [if bits & U_SCALE] scale = ReadByte() / 16.f
- [if bits & U_INSTANCE] ReadByte(instance_bits)
- [if bits & U_OWNER] ReadShort(owner)
- [if bits & U_OLDFRAME] ReadUShort(old_frame)

## (packet, server -> client) svc_splitclient (21)

This packet indicates to the client which split screen player the next messages are directed towards, for unicast messages.

- ReadByte (isplit)

Note that `isplit` will be offset by 1 (that is to say, a value of 1 indicates split screen client 0).

## (packet, server -> client) svc_configblast (22)

Compressed configstring data. This is to make connection faster by sending fewer packets.

- ReadShort (compressed size)
- ReadShort (uncompressed size)
- ReadByte[compressed size] (buffer)

The received `buffer` is directly passed through to zlib's `uncompress`. After decompression, until the buffer is exhausted, the following data repeats:
- ReadUShort (index)
- ReadString (str) 

## (packet, server -> client) svc_spawnbaselineblast (23)

Compressed baseline data. This is to make connection faster by sending fewer packets.

- ReadShort (compressed size)
- ReadShort (uncompressed size)
- ReadByte[compressed size] (buffer)

The received `buffer` is directly passed through to zlib's `uncompress`. After decompression, until the buffer is exhausted, read in the data contained in a `svc_spawnbaseline` packet.

## (packet, server -> client) svc_level_restart (24)

Sent when the server executes a `restart_level` command. The client should be prepared to do a "soft wipe" of their state, but might want to defer it until the full frame is read since effects might come in after this command is executed.

This message's data contains configstrings that were changed by restarting the level. The following should be repeated until an exit condition is met:

- ReadShort (id)
- [if id is -1, exit]
- ReadString (str)

## (packet, server -> client) svc_damage (25)

This message is sent after accumulating damage on a player. It gives the player a rough idea of the damage they're receiving and from where.

- ReadByte (count)

For `count` number of loops, read the following:

- ReadByte (encoded)
- ReadDir

`encoded` is in the following format:
```
struct packed_damage_t
{
	uint8_t damage : 5;
	uint8_t health : 1;
	uint8_t armor : 1;
	uint8_t shield : 1;
}
```

`health` provides a `1,0,0` addition to color.
`armor` provides a `1,1,1` addition to color.
`shield` provides a `0,1,0` addition to color.

The `damage` value is also divided by 3, so multiplying it by 3 will get you an approximation of the real damage amount.

The color is then normalized.

## (packet, server -> client) svc_locprint (26)

This packet is the new entry point for prints.

- ReadByte (flags)
- ReadString (base)
- ReadByte (num args)
- ReadString[num args] (args)

The `base` string is a `fmtlib` formatted string.

The information in [Print Adjustments](#print-adjustments) and [Loc_Print](#loc_print) explains how formatting works.

## (packet, server -> client) svc_fog (27)

```cpp
enum bits_t : uint16_t
{
	// global fog
	BIT_DENSITY     = bit_v<0>,
	BIT_R           = bit_v<1>,
	BIT_G           = bit_v<2>,
	BIT_B           = bit_v<3>,
	BIT_TIME        = bit_v<4>, // if set, the transition takes place over N milliseconds

	// height fog
	BIT_HEIGHTFOG_FALLOFF   = bit_v<5>,
	BIT_HEIGHTFOG_DENSITY   = bit_v<6>,
	BIT_MORE_BITS           = bit_v<7>, // read additional bit
	BIT_HEIGHTFOG_START_R   = bit_v<8>,
	BIT_HEIGHTFOG_START_G   = bit_v<9>,
	BIT_HEIGHTFOG_START_B   = bit_v<10>,
	BIT_HEIGHTFOG_START_DIST= bit_v<11>,
	BIT_HEIGHTFOG_END_R     = bit_v<12>,
	BIT_HEIGHTFOG_END_G     = bit_v<13>,
	BIT_HEIGHTFOG_END_B     = bit_v<14>,
	BIT_HEIGHTFOG_END_DIST  = bit_v<15>
};
```

- ReadByte (bits)
- [if bits & BIT_MORE_BITS] ReadByte (morebits), bits |= (morebits \<\< 8)
- [if bits & BIT_DENSITY] ReadFloat (density)
- [if bits & BIT_DENSITY] ReadByte (skyfactor)
- [if bits & BIT_R] ReadByte (red)
- [if bits & BIT_G] ReadByte (green)
- [if bits & BIT_B] ReadByte (blue)
- [if bits & BIT_TIME] ReadUShort (time)
- [if bits & BIT_HEIGHTFOG_FALLOFF] ReadFloat (heightfog falloff)
- [if bits & BIT_HEIGHTFOG_DENSITY] ReadFloat (heightfog density)
- [if bits & BIT_HEIGHTFOG_START_R] ReadByte (heightfog start red)
- [if bits & BIT_HEIGHTFOG_START_G] ReadByte (heightfog start green)
- [if bits & BIT_HEIGHTFOG_START_B] ReadByte (heightfog start blue)
- [if bits & BIT_HEIGHTFOG_START_DIST] ReadLong (heightfog start distance)
- [if bits & BIT_HEIGHTFOG_END_R] ReadByte (heightfog end red)
- [if bits & BIT_HEIGHTFOG_END_G] ReadByte (heightfog end green)
- [if bits & BIT_HEIGHTFOG_END_B] ReadByte (heightfog end blue)
- [if bits & BIT_HEIGHTFOG_END_DIST] ReadLong (heightfog end distance)

## (packet, server -> client) svc_waitingforplayers (28)

Sent when there are players waiting to join before the game can start (or zero if all players are in).

- ReadByte (count)

## (packet, server -> client) svc_bot_chat (29)

Bots talking to players.

- ReadString (bot name)
- ReadShort (client index, or 256 if no particular player)
- ReadString (loc string)

## (packet, server -> client) svc_poi (30)

Spawn a POI.

```cpp
enum svc_poi_flags
{
    POI_FLAG_NONE = 0,
    POI_FLAG_HIDE_ON_AIM = 1, // hide the POI if we get close to it with our aim
};
```

- ReadUShort (key)
- ReadUShort (time)
- ReadPos (pos)
- ReadUShort (image index)
- ReadByte (palette index)
- ReadByte (flags)

If a non-zero `key` is specified, only one of that POI key can exist at any given time. If `time` is 0xFFFF, the POI that matches the key will be removed.

If `time` is zero, the POI will last forever, `key` should be set in order to allow the POI to be cleaned up later. 

## (packet, server -> client) svc_help_path (31)

Spawns the Compass help path effect at the given location.

- ReadByte (start)
- ReadPos (pos)
- ReadDir (dir)

## (packet, server -> client) svc_achievement (32)

- ReadString (id)

## (packet, client -> server) clc_stringcmd (4)

- ReadByte (isplit)
- ReadString (s)

Note that `isplit` is offset by 1, so `1` is the first split screen client.