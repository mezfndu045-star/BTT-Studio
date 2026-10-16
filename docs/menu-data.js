// The in-game menu (src/menu/Menu.cpp, initCategories) for the site's interactive menu (menu.js).
// Each category: name, split (index of the first item of the right column, or none), items.
// Each item: t = title, kind (bool | enum | int | button | key | sub), v = default value, o = choices, h = help.
const KEYS = ['Disabled', 'None', 'L', 'R', 'ZL', 'ZR', 'L + R', 'L + ZL', 'L + ZR', 'R + ZL', 'R + ZR', 'Custom'];
const TP_KEYS = ['D-Pad Left', 'D-Pad Right', 'D-Pad Down', 'D-Pad Up', 'L + D-Pad Left', 'L + D-Pad Right', 'L + D-Pad Down', 'L + D-Pad Up', 'ZL + D-Pad Left', 'ZL + D-Pad Right', 'ZL + D-Pad Down', 'ZL + D-Pad Up', 'R + D-Pad Left', 'R + D-Pad Right', 'LStick', 'RStick', 'L + LStick', 'L + RStick', 'ZL + ZR', 'Disabled', 'Custom'];
const MENU_KEYS = ['L + DLeft', 'L + DRight', 'R + DLeft', 'R + DRight', 'Custom'];
const TIMER_TYPES = ['None', 'Press Start', 'Odyssey Power Up', 'Black Screen', 'White Screen', 'Capture', 'Damage',
  'Escape Wire', 'Moon Get', 'Shine Tick', '2 Moons (Grab)', '2 Moons (Tick)', 'Shine Grab'];
const COLORS = ['None', 'White', 'Gray', 'Black', 'Red', 'Blue', 'Yellow', 'Green', 'Orange', 'Purple', 'Pink', 'Light Blue', 'Multicolor'];
const KINGDOMS = ['Cap Kingdom', 'Cascade Kingdom', 'Sand Kingdom', 'Wooded Kingdom', 'Lake Kingdom', 'Cloud Kingdom',
  'Lost Kingdom', 'Metro Kingdom', 'Snow Kingdom', 'Seaside Kingdom', 'Luncheon Kingdom', 'Ruined Kingdom',
  "Bowser's Kingdom", 'Moon Kingdom', 'Mushroom Kingdom', 'Dark Side', 'Darker Side'];
const SUB_AREA_KINGDOMS = ['Cap', 'Cascade', 'Sand', 'Wooded', 'Lake', 'Cloud', 'Lost', 'Metro', 'Snow', 'Seaside',
  'Luncheon', 'Ruined', 'Bowser', 'Moon', 'Mushroom', 'Dark Side', 'Darker Side'];
const WHEEL_ACTIONS = ['-', 'Moon Refresh', 'Gray Moon Refresh', 'Noclip', 'No Damage', 'Restart Scene', 'Kill Mario',
  'Heal Mario', 'Warp Last CP', 'Start Timer', 'Reset Timer'];

const b = (t, h, v = false) => ({ t, kind: 'bool', v, h });
const e = (t, o, v, h) => ({ t, kind: 'enum', o, v, h });
const n = (t, v, min, max, step, h) => ({ t, kind: 'int', v, min, max, step, h });
const btn = (t, h) => ({ t, kind: 'button', h });

const MENU = [
  {
    name: 'Options', split: 15,
    items: [
      b('Moon Refresh', 'Moons you collect still count (moon counter, story) but come back as new moons: you can collect the same moon again as many times as you want.'),
      b('Gray Moon Refresh', 'Moons already collected look normal again (not gray), as if they were never collected.'),
      b('Purple Coins Refresh', 'Purple coins come back when the scene is reloaded.'),
      b('Doors Refresh', 'Doors already opened are closed again when the scene is reloaded.'),
      b('Moon Shards Refresh', 'Moon shards (and their counter) come back, even when their moon is already collected.'),
      b('Kingdom Enter Refresh', 'The Odyssey arrival cutscene of the kingdom plays again.'),
      b('Seeds Refresh', 'Seeds come back and pots are empty at each reload. The plant grows normally (with time or water).'),
      b('Seeds Growing', 'A planted seed grows to the end at once and its moon appears. Works with or without Seeds Refresh.'),
      b('Cutscene Refresh', 'Cutscenes and first talks of the characters (Toads, Peach, Captain Toad...) play again, with the camera with the story text at the start of a kingdom.'),
      b('Text Refresh', "Cappy's texts (at the start of the kingdoms, the checkpoint flags explanation...) come back at each reload. Off: they never show."),
      b('Always Manual Cutscene', 'Cutscenes never skip by themselves: you choose when to skip them.'),
      b('Always Checkpoints', 'You can always warp to the flags from the map, even where the game normally forbids it.'),
      b('Disable Autosave', 'The game no longer saves by itself: your save file stays the same while you practice.'),
      b('Disable TP Puppet', 'Mario is not placed automatically during the moon get animation.'),
      b('Skip Cloud', 'In the Cloud Kingdom, Bowser is beaten as soon as his intro cutscene ends: the battle end cutscene plays right away.'),
      b('All Checkpoints', 'Every flag of the kingdom is available on the map, even the ones not touched yet.'),
      b('Luigi Pixels', 'The 8-bit Luigi hidden in some bushes always comes out (10% chance in the normal game). Reload the scene after turning it on.'),
      b('Spawn All Hint Art', 'Every hint art appears, without having to unlock it.'),
      b('Pipe Maze Override', 'Forces the path of the pipe mazes (the same layout every time).'),
      b('No Checkpoint Touch', 'Touching a flag does not activate it (your tries are not changed by a flag touched by mistake).'),
      b('No Damage', 'Mario no longer loses health.'),
      b('Disable Music', 'Turns the music off: only the sound effects stay.'),
      b('Disable Moon Lock', 'The Odyssey can leave without the number of moons the kingdom normally asks for.'),
      b('Skip Broodals', 'As soon as the intro of a Broodal fight ends (Hariet, Rango, Spewart, Madame Broode), the boss is beaten and its Multi Moon appears.'),
      b('Always Odyssey Power Up', 'Each time you go to the Odyssey, even without moons, it powers up and offers the next kingdom of the story.'),
      b('Odyssey In All Kingdoms', 'The Odyssey is in every kingdom, even the ones your save has not reached yet, and you can warp to it from the map.'),
      b('Night Metro Multi Moon', 'At the start of Night Metro, the Mechawiggler Multi Moon falls next to Mario: grab it to go straight to day Metro.'),
      b('Open Outfit Doors', 'The guards of the outfit rooms let Mario in whatever outfit he wears.'),
      b('Noclip', 'Mario flies through everything. Left stick = move, ZR = up, ZL = down, X/Y = faster, A/B = slower. Turns off at each game launch.'),
      b('Reload Scene at Pos', 'When the scene is reloaded, Mario comes back exactly where he was.'),
    ],
  },
  {
    name: 'Stages',
    items: [
      e('Kingdom', KINGDOMS, 0, 'The kingdom to warp to (17 kingdoms). A picture of the kingdom shows next to the menu.'),
      n('Scenario', 0, 0, 15, 1, 'The story scenario of the kingdom. 0: the scenario of your save.'),
      btn('Warp to Stage', 'Warps right away to the chosen kingdom and scenario. Also works from a new save: a kingdom not unlocked yet gets unlocked like with the Odyssey.'),
      ...SUB_AREA_KINGDOMS.map((k) => ({ t: `${k} Sub Area`, kind: 'sub', h: `Every sub-area of the ${k === 'Dark Side' || k === 'Darker Side' ? k : k + ' Kingdom'} (secret rooms, shops, towers, fights...). A to choose, Left / Right to change the area, A to warp there, B to cancel. Two pictures show the entrance seen from outside and the inside of the area.` })),
    ],
  },
  {
    name: 'Misc', split: 14,
    items: [
      e('Outfit Body', ['Mario', 'Mario64', 'Mario64Metal', 'MarioAloha', 'MarioArmor', 'MarioBone', 'MarioClown', 'MarioColorClassic', 'MarioColorGold', 'MarioColorLuigi', 'MarioColorWaluigi', 'MarioColorWario', 'MarioCook', 'MarioDiddyKong', 'MarioDoctor', 'MarioExplorer', 'MarioFootball', 'MarioGolf', 'MarioGunman', 'MarioHakama', 'MarioHappi', 'MarioKing', 'MarioKoopa', 'MarioMaker', 'MarioMechanic', 'MarioNew3DS', 'MarioPainter', 'MarioPeach', 'MarioPilot', 'MarioPirate', 'MarioPoncho', 'MarioPrimitiveMan', 'MarioSailor', 'MarioScientist', 'MarioShopman', 'MarioSnowSuit', 'MarioSpaceSuit', 'MarioSuit', 'MarioSwimwear', 'MarioTailCoat', 'MarioTuxedo', 'MarioUnderwear'], 0, 'The outfit to put on (42 outfits).'),
      e('Outfit Cap', ['Mario', 'Mario64', 'Mario64Metal', 'MarioAloha', 'MarioArmor', 'MarioBone', 'MarioClown', 'MarioColorClassic', 'MarioColorGold', 'MarioColorLuigi', 'MarioColorWaluigi', 'MarioColorWario', 'MarioCook', 'MarioDiddyKong', 'MarioDoctor', 'MarioExplorer', 'MarioFootball', 'MarioGolf', 'MarioGunman', 'MarioHakama', 'MarioHappi', 'MarioKing', 'MarioKoopa', 'MarioMaker', 'MarioMechanic', 'MarioNew3DS', 'MarioPainter', 'MarioPeach', 'MarioPilot', 'MarioPirate', 'MarioPoncho', 'MarioPrimitiveMan', 'MarioSailor', 'MarioScientist', 'MarioShopman', 'MarioSnowSuit', 'MarioSpaceSuit', 'MarioSuit', 'MarioSwimwear', 'MarioTailCoat', 'MarioTuxedo', 'MarioInvisible'], 0, 'The hat to put on (42 hats).'),
      btn('Apply Outfit', 'Puts the chosen outfit and hat on right away.'),
      btn('Save Position', "Saves Mario's current position."),
      btn('Load Position', 'Teleports Mario to the saved position, with the camera on him instantly.'),
      btn('Restart Scene', 'Restarts the scene from its normal starting point.'),
      b('Reload at Pos', 'Reloading the scene puts Mario back where he was instead of the start.'),
      btn('Restart at Pos', 'Reloads the scene now, with Mario back where he is.'),
      btn('Kill Mario', 'Mario dies right away.'),
      btn('Damage Mario', 'Mario loses 1 health point.'),
      btn('Life Up Heart', 'Gives Mario a Life-Up Heart (6 health points).'),
      btn('Heal Mario', "Restores all of Mario's health."),
      btn('Add 1000 Coins', 'Adds 1000 coins.'),
      btn('Remove 1000 Coins', 'Removes 1000 coins.'),
      btn('Reload Scene', 'Reloads the scene.'),
      btn('Previous Scene', 'Goes back to the previous area or kingdom.'),
      e('Wiggler Pattern', ['Random', 'Ghost', 'Nose', 'C', 'W', 'J', 'Medal', 'Plane', '5', 'Hangman', 'Spanish', 'Siblings', 'Snake', '8', 'Mushroom', 'Z', 'Tetris', 'Ear', 'Bomb', 'Bird', 'L', 'O', 'Star'], 0, 'The attack pattern of Mechawiggler (23 choices).'),
      btn('Rename All Moons', 'Type a name with the on-screen keyboard: every moon takes that name (when you get it, on the map...).'),
      btn('Reset All Moon Names', 'Moons get their real names back.'),
    ],
  },
  {
    name: 'Timer', split: 13,
    items: [
      b('Segment Timer', 'Shows the timer on screen.'),
      b('Timer Split', 'Instead of stopping, the timer shows a split time.'),
      e('Start Type', TIMER_TYPES, 0, 'The event that starts the timer (from zero). For example Odyssey Power Up: when the screen goes black after "The Odyssey has powered up" (or when it is repaired).'),
      n('Start Count', 1, 1, 99, 1, 'How many times the Start Type event must happen to start the timer.'),
      e('End Type', TIMER_TYPES, 0, 'The event that stops the timer. With the same type as the Start Type, it starts and stops in turn.'),
      n('End Count', 1, 1, 99, 1, 'How many times the End Type event must happen to stop the timer. Example: Moon Get + 3 = the timer stops at the 3rd moon.'),
      btn('Start Timer', 'Starts the timer by hand.'),
      btn('Stop Timer', 'Stops the timer by hand.'),
      btn('Reset Timer', 'Puts the timer back to zero.'),
      b('Splits', 'Each End Type event (after End Count) goes to the next split, the last split ends the run. Under the timer, each split shows the time when it ended.'),
      n('Split Count', 5, 2, 30, 1, 'How many splits the run has (2 to 30).'),
      n('Edit Split', 1, 1, 30, 1, 'The split whose end type is set with Split Type.'),
      e('Split Type', ['End Type', ...TIMER_TYPES], 0, 'The event that ends the split chosen in Edit Split. "End Type": the End Type of the timer.'),
      e('Preset Pos', ['Center', 'Top Left', 'Top Center', 'Top Right', 'Bottom Left', 'Bottom Right'], 0, 'Puts the timer in one of these places. You can also drag it with the mouse while the menu is open.'),
      n('Position X', 800, 0, 1600, 25, 'Horizontal position of the timer.'),
      n('Position Y', 450, 0, 900, 25, 'Vertical position of the timer.'),
      btn('Reset Position', 'Puts the timer back in its default place.'),
    ],
  },
  {
    name: 'Input Display', split: 10,
    items: [
      b('Input Display', 'Shows the controller and the buttons you press on screen.', true),
      b('2P Mode', 'Also shows the 2nd controller (2 player mode, Cappy).'),
      e('Style', ['Classic', 'Periscope', 'Minimal', 'Switch Pro', 'Xbox', 'PS4', 'GameCube', 'N64', '...', 'Motion'], 0, 'Look of the controller: Classic, Periscope, Minimal, 109 Open Joystick Display styles, and "Motion": a 3D controller that turns like the real one (gyroscope).'),
      { t: 'Reset Motion', kind: 'key', v: 'Custom (L + R)', h: 'Style "Motion": the key that makes the current orientation of the controller the front view.' },
      e('Motion Controller', ['Auto', 'Pro Controller', 'Joy-Con'], 0, 'Style "Motion": the controller whose motion is shown, and its shape. Auto: the controller in use (Pro Controller, or Joy-Con).'),
      e('Preset Pos', ['Center', 'Bottom Left', 'Bottom Right', 'Top Left', 'Top Right'], 0, 'Puts the controller in the center or in a corner. You can also drag it with the mouse while the menu is open.'),
      n('Position X', 800, 0, 1600, 25, 'Horizontal position of the controller.'),
      n('Position Y', 450, 0, 900, 25, 'Vertical position of the controller.'),
      btn('Reset Position', 'Puts the controller back in the center.'),
      n('Size', 100, 30, 150, 10, 'Size of the controller, in % (30 to 150), for both controllers.'),
      e('P2 Preset Pos', ['Center', 'Bottom Left', 'Bottom Right', 'Top Left', 'Top Right'], 0, 'Same as Preset Pos, for the 2nd controller.'),
      n('P2 Position X', 800, 0, 1600, 25, 'Horizontal position of the 2nd controller.'),
      n('P2 Position Y', 450, 0, 900, 25, 'Vertical position of the 2nd controller.'),
      btn('P2 Reset Position', 'Puts the 2nd controller back in the center.'),
      e('Button Color', COLORS, 1, 'Color of the buttons at rest.'),
      e('Pressed Color', COLORS, 10, 'Color of a button while it is pressed.'),
      e('Stick Color', COLORS, 1, 'Color of the sticks.'),
      e('Ring Color', COLORS, 2, 'Color of the ring around the sticks.'),
      e('BG Color', COLORS, 3, 'Color of the background behind the controller. Multicolor: an animated rainbow.'),
    ],
  },
  {
    name: 'Keybinds', split: 11,
    items: [
      e('Open Menu', MENU_KEYS, 0, 'The buttons that open and close this menu. L + D-Pad Left always works. Custom: any combination (press A, hold the buttons, release them).'),
      e('Save Position', TP_KEYS, 0, "Saves Mario's position."),
      e('Load Position', TP_KEYS, 1, 'Teleports Mario to the saved position.'),
      e('Restart Scene', TP_KEYS, 3, 'Restarts the scene.'),
      e('Heal Mario', KEYS, 0, "Restores Mario's health. The D-Pad Up shortcuts: \"None\" = D-Pad Up alone, \"L\" = L + D-Pad Up..."),
      e('Life Up', KEYS, 0, 'Gives the Life-Up Heart.'),
      e('Prev Scene', KEYS, 0, 'Goes back to the previous scene.'),
      e('Warp Last CP', KEYS, 0, 'Warps to the last flag you touched.'),
      e('Timer Start', KEYS, 0, 'Starts the timer.'),
      e('Timer End', KEYS, 0, 'Stops the timer.'),
      e('Timer Reset', KEYS, 0, 'Puts the timer back to zero.'),
      e('Wheel Key', ['Off', 'L3', 'R3', 'L3 or R3', 'L (hold)', 'R (hold)'], 2, 'The button of the quick wheel: point at an action with the right stick, release (or click again) to run it.'),
      e('Wheel Size', ['6', '8', '12'], 1, 'How many slots the quick wheel has.'),
      ...Array.from({ length: 12 }, (_, i) => e(`Wheel Slot ${i + 1}`, WHEEL_ACTIONS, 0, 'The action of this slot of the quick wheel: any option or button of Options, Misc and Timer.')),
    ],
  },
  {
    name: 'Info',
    items: [
      b('Info Window', 'Shows a window with live data.'),
      b('Player Info', "Shows Mario's position (X, Y, Z), speed and what Cappy captured."),
    ],
  },
  {
    name: 'Settings',
    items: [
      e('Menu Corner', ['Top Left', 'Top Right', 'Bottom Left', 'Bottom Right'], 0, 'The corner of the screen the menu sticks to.'),
      e('Language', ['English', 'Français'], 0, 'Language of the menu.'),
    ],
  },
];
