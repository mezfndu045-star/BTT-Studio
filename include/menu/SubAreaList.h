#pragma once

// Generated from SystemData/WorldList.szs (WorldListFromDb.byml): every sub area of each kingdom, in the order of
// the Stages > Kingdom list. Preview images: romfs ImGuiData/SubAreas/<stage>.astc

namespace btt {

struct SubArea {
    const char* stage;
    const char* name;
};

struct KingdomSubAreas {
    const char* title;
    const SubArea* areas;
    const char* const* names;
    int count;
};

inline constexpr SubArea sSubAreasCap[] = {
    { "FrogSearchExStage", "Frog Pond" },
    { "CapWorldTowerStage", "Inside Top-Hat Tower" },
    { "PoisonWaveExStage", "Poison Tides" },
    { "RollingExStage", "Precision Rolling" },
    { "PushBlockExStage", "Push-Block Peril" },
};
inline constexpr const char* sSubAreasCapNames[] = {
    "Frog Pond",
    "Inside Top-Hat Tower",
    "Poison Tides",
    "Precision Rolling",
    "Push-Block Peril",
};
inline constexpr SubArea sSubAreasWaterfall[] = {
    { "Lift2DExStage", "8-Bit Chasm Lifts" },
    { "WanwanClashExStage", "Chain Chomp Shots" },
    { "TrexPoppunExStage", "Dinosaur Nest" },
    { "WindBlowExStage", "Gusty Bridges" },
    { "CapAppearExStage", "Mysterious Clouds" },
};
inline constexpr const char* sSubAreasWaterfallNames[] = {
    "8-Bit Chasm Lifts",
    "Chain Chomp Shots",
    "Dinosaur Nest",
    "Gusty Bridges",
    "Mysterious Clouds",
};
inline constexpr SubArea sSubAreasSand[] = {
    { "SandWorldKillerExStage", "Bullet Bill Maze" },
    { "RocketFlowerExStage", "Colossal Ruins" },
    { "SandWorldCostumeStage", "Costume Room" },
    { "SandWorldShopStage", "Crazy Cap Store" },
    { "SandWorldUnderground001Stage", "Deepest Underground" },
    { "WaterTubeExStage", "Freezing Waterway" },
    { "SandWorldPressExStage", "Ice Cave" },
    { "SandWorldPyramid000Stage", "Inverted Pyramid: Lower" },
    { "SandWorldPyramid001Stage", "Inverted Pyramid: Upper" },
    { "SandWorldSphinxExStage", "Jaxi Driving" },
    { "SandWorldVibrationStage", "Rumbling Floor House" },
    { "SandWorldSecretStage", "Sphynx Treasure Vault" },
    { "SandWorldRotateExStage", "Strange Neighborhood" },
    { "SandWorldMeganeExStage", "The Invisible Maze" },
    { "SandWorldSlotStage", "Tostarena Slots" },
    { "MeganeLiftExStage", "Transparent Lifts" },
    { "SandWorldUnderground000Stage", "Underground Ruins" },
};
inline constexpr const char* sSubAreasSandNames[] = {
    "Bullet Bill Maze",
    "Colossal Ruins",
    "Costume Room",
    "Crazy Cap Store",
    "Deepest Underground",
    "Freezing Waterway",
    "Ice Cave",
    "Inverted Pyramid: Lower",
    "Inverted Pyramid: Upper",
    "Jaxi Driving",
    "Rumbling Floor House",
    "Sphynx Treasure Vault",
    "Strange Neighborhood",
    "The Invisible Maze",
    "Tostarena Slots",
    "Transparent Lifts",
    "Underground Ruins",
};
inline constexpr SubArea sSubAreasForest[] = {
    { "KillerRoadExStage", "Breakdown Road" },
    { "ForestWorldWoodsCostumeStage", "Costume Room" },
    { "ShootingElevatorExStage", "Crowded Elevator" },
    { "ForestWorldWoodsStage", "Deep Woods" },
    { "ForestWorldWoodsTreasureStage", "Deep Woods Treasure Vault" },
    { "ForestWorldWaterExStage", "Flooding Pipeway" },
    { "RailCollisionExStage", "Flower Road" },
    { "AnimalChaseExStage", "Herding Sheep" },
    { "PackunPoisonExStage", "Invisible Road" },
    { "ForestWorldBossStage", "Secret Flower Field" },
    { "FogMountainExStage", "Shards in the Fog" },
    { "ForestWorldTowerStage", "Sky Garden Tower" },
    { "ForestWorldBonusStage", "Spinning-Platforms Vault" },
    { "ForestWorldCloudBonusExStage", "Walking on Clouds" },
};
inline constexpr const char* sSubAreasForestNames[] = {
    "Breakdown Road",
    "Costume Room",
    "Crowded Elevator",
    "Deep Woods",
    "Deep Woods Treasure Vault",
    "Flooding Pipeway",
    "Flower Road",
    "Herding Sheep",
    "Invisible Road",
    "Secret Flower Field",
    "Shards in the Fog",
    "Sky Garden Tower",
    "Spinning-Platforms Vault",
    "Walking on Clouds",
};
inline constexpr SubArea sSubAreasLake[] = {
    { "LakeWorldShopStage", "Crazy Cap Store" },
    { "TrampolineWallCatchExStage", "Jump, Grab, Cling, Climb" },
    { "GotogotonExStage", "Puzzle Room" },
    { "FastenerExStage", "Unzipping the Chasm" },
    { "FrogPoisonExStage", "Waves of Poison" },
};
inline constexpr const char* sSubAreasLakeNames[] = {
    "Crazy Cap Store",
    "Jump, Grab, Cling, Climb",
    "Puzzle Room",
    "Unzipping the Chasm",
    "Waves of Poison",
};
inline constexpr SubArea sSubAreasCloud[] = {
    { "Cube2DExStage", "2D Cube" },
    { "FukuwaraiKuriboStage", "Picture Match (Goomba)" },
};
inline constexpr const char* sSubAreasCloudNames[] = {
    "2D Cube",
    "Picture Match (Goomba)",
};
inline constexpr SubArea sSubAreasClash[] = {
    { "ClashWorldShopStage", "Crazy Cap Store" },
    { "JangoExStage", "Klepto Lava Room" },
    { "ImomuPoisonExStage", "Stretch Jungle" },
};
inline constexpr const char* sSubAreasClashNames[] = {
    "Crazy Cap Store",
    "Klepto Lava Room",
    "Stretch Jungle",
};
inline constexpr SubArea sSubAreasCity[] = {
    { "PoleKillerExStage", "Bullet Billding" },
    { "CityWorldShop01Stage", "Crazy Cap Store" },
    { "CityPeopleRoadStage", "Crowded Alleyway" },
    { "CityWorldMainTowerStage", "Inside City Hall" },
    { "CityWorldSandSlotStage", "Metro Kingdom Slots" },
    { "Theater2DExStage", "On the Big Screen" },
    { "DonsukeExStage", "Pitchblack Mountain" },
    { "CityWorldFactoryStage", "Power Plant" },
    { "Note2D3DRoomExStage", "Rainbow Notes" },
    { "RadioControlExStage", "RC Race" },
    { "ElectricWireExStage", "Rewiring the Neighborhood" },
    { "CapRotatePackunExStage", "Rotating Maze" },
    { "BikeSteelExStage", "Scooter Daredevil" },
    { "TrexBikeExStage", "Scooter Escape" },
    { "ShootingCityExStage", "Shards Under Siege" },
    { "PoleGrabCeilExStage", "Swinging High-Rises" },
    { "SwingSteelExStage", "Swinging Scaffolding" },
};
inline constexpr const char* sSubAreasCityNames[] = {
    "Bullet Billding",
    "Crazy Cap Store",
    "Crowded Alleyway",
    "Inside City Hall",
    "Metro Kingdom Slots",
    "On the Big Screen",
    "Pitchblack Mountain",
    "Power Plant",
    "Rainbow Notes",
    "RC Race",
    "Rewiring the Neighborhood",
    "Rotating Maze",
    "Scooter Daredevil",
    "Scooter Escape",
    "Shards Under Siege",
    "Swinging High-Rises",
    "Swinging Scaffolding",
};
inline constexpr SubArea sSubAreasSnow[] = {
    { "SnowWorldLobby000Stage", "Bound Bowl Lobby" },
    { "SnowWorldLobby001Stage", "Bound Bowl Lobby (Race)" },
    { "SnowWorldLobbyExStage", "Class S Lobby" },
    { "SnowWorldCostumeStage", "Cold Room" },
    { "SnowWorldShopStage", "Crazy Cap Store" },
    { "IceWaterDashExStage", "Dashing Over Cold Water" },
    { "IceWaterBlockExStage", "Freezing Water" },
    { "SnowWorldTownStage", "Shiveria Town" },
    { "SnowWorldCloudBonusExStage", "Spinning Above the Clouds" },
    { "ByugoPuzzleExStage", "Ty-Foo Sliding Puzzle" },
    { "IceWalkerExStage", "Walking on Ice" },
    { "KillerRailCollisionExStage", "Wintery Flower Road" },
};
inline constexpr const char* sSubAreasSnowNames[] = {
    "Bound Bowl Lobby",
    "Bound Bowl Lobby (Race)",
    "Class S Lobby",
    "Cold Room",
    "Crazy Cap Store",
    "Dashing Over Cold Water",
    "Freezing Water",
    "Shiveria Town",
    "Spinning Above the Clouds",
    "Ty-Foo Sliding Puzzle",
    "Walking on Ice",
    "Wintery Flower Road",
};
inline constexpr SubArea sSubAreasSea[] = {
    { "SeaWorldCostumeStage", "Costume Room" },
    { "WaterValleyExStage", "Narrow Valley" },
    { "SeaWorldVibrationStage", "Rumbling Floor Cave" },
    { "CloudExStage", "Sea of Clouds" },
    { "SenobiTowerExStage", "Sinking Island" },
    { "SeaWorldSecretStage", "Sphynx Underwater Vault" },
    { "TogezoRotateExStage", "Spinning Maze" },
    { "ReflectBombExStage", "Stop, Poke and Roll" },
    { "SeaWorldSneakingManStage", "Wriggling Moon Cave" },
};
inline constexpr const char* sSubAreasSeaNames[] = {
    "Costume Room",
    "Narrow Valley",
    "Rumbling Floor Cave",
    "Sea of Clouds",
    "Sinking Island",
    "Sphynx Underwater Vault",
    "Spinning Maze",
    "Stop, Poke and Roll",
    "Wriggling Moon Cave",
};
inline constexpr SubArea sSubAreasLava[] = {
    { "GabuzouClockExStage", "Blazing Gears" },
    { "LavaWorldExcavationExStage", "Cheese Rocks" },
    { "LavaWorldShopStage", "Crazy Cap Store" },
    { "ForkExStage", "Fork Flickin" },
    { "LavaWorldFenceLiftExStage", "Lava Island Fly-By" },
    { "LavaWorldUpDownExStage", "Magma Swamp" },
    { "LavaWorldBubbleLaneExStage", "Narrow Magma Path" },
    { "LavaWorldCostumeStage", "Simmering Kitchen" },
    { "LavaBonus1Zone", "Slots" },
    { "LavaWorldClockExStage", "Spinning Athletics" },
    { "LavaWorldTreasureStage", "Veggie Treasure Room" },
    { "CapAppearLavaLiftExStage", "Volcano Cave" },
};
inline constexpr const char* sSubAreasLavaNames[] = {
    "Blazing Gears",
    "Cheese Rocks",
    "Crazy Cap Store",
    "Fork Flickin",
    "Lava Island Fly-By",
    "Magma Swamp",
    "Narrow Magma Path",
    "Simmering Kitchen",
    "Slots",
    "Spinning Athletics",
    "Veggie Treasure Room",
    "Volcano Cave",
};
inline constexpr SubArea sSubAreasAttack[] = {
    { "BullRunExStage", "Mummy Army" },
    { "DotTowerExStage", "Roulette Tower" },
};
inline constexpr const char* sSubAreasAttackNames[] = {
    "Mummy Army",
    "Roulette Tower",
};
inline constexpr SubArea sSubAreasSky[] = {
    { "SkyWorldShopStage", "Crazy Cap Store" },
    { "SkyWorldCloudBonusExStage", "Dashing Above Clouds" },
    { "SkyWorldCostumeStage", "Folding Screen" },
    { "KaronWingTowerStage", "Hexagon Tower" },
    { "JizoSwitchExStage", "Jizo's Adventure" },
    { "TsukkunRotateExStage", "Spinning Tower" },
    { "SkyWorldTreasureStage", "Treasure Vault" },
    { "TsukkunClimbExStage", "Wooden Tower" },
};
inline constexpr const char* sSubAreasSkyNames[] = {
    "Crazy Cap Store",
    "Dashing Above Clouds",
    "Folding Screen",
    "Hexagon Tower",
    "Jizo's Adventure",
    "Spinning Tower",
    "Treasure Vault",
    "Wooden Tower",
};
inline constexpr SubArea sSubAreasMoon[] = {
    { "Galaxy2DExStage", "8-Bit Galaxy" },
    { "MoonWorldKoopa1Stage", "Bowser Fight" },
    { "MoonWorldKoopa2Stage", "Bowser Fight 2" },
    { "MoonWorldShopRoom", "Crazy Cap Store" },
    { "MoonAthleticExStage", "Giant Swings" },
    { "MoonWorldBasementStage", "Moon Cave Escape" },
    { "MoonWorldSphinxRoom", "Sphynx Vault" },
    { "MoonWorldCaptureParadeStage", "Underground Caverns" },
    { "MoonWorldWeddingRoomStage", "Wedding Room" },
    { "MoonWorldWeddingRoom2Stage", "Wedding Room (Rematch)" },
};
inline constexpr const char* sSubAreasMoonNames[] = {
    "8-Bit Galaxy",
    "Bowser Fight",
    "Bowser Fight 2",
    "Crazy Cap Store",
    "Giant Swings",
    "Moon Cave Escape",
    "Sphynx Vault",
    "Underground Caverns",
    "Wedding Room",
    "Wedding Room (Rematch)",
};
inline constexpr SubArea sSubAreasPeach[] = {
    { "DotHardExStage", "8-Bit Bullet Bills" },
    { "PeachWorldCostumeStage", "Castle Courtyard 64" },
    { "PeachWorldShopStage", "Crazy Cap Store" },
    { "PeachWorldPictureBossMagmaStage", "Painting: Cookatiel" },
    { "PeachWorldPictureBossKnuckleStage", "Painting: Knucklotec" },
    { "PeachWorldPictureBossRaidStage", "Painting: Lord of Lightning" },
    { "PeachWorldPictureMofumofuStage", "Painting: Mechawiggler" },
    { "PeachWorldPictureGiantWanderBossStage", "Painting: Mollusque" },
    { "PeachWorldPictureBossForestStage", "Painting: Torkdrift" },
    { "PeachWorldCastleStage", "Peach's Castle" },
    { "FukuwaraiMarioStage", "Picture Match (Mario)" },
    { "RevengeBossMagmaStage", "Rematch: Cookatiel" },
    { "RevengeBossKnuckleStage", "Rematch: Knucklotec" },
    { "RevengeBossRaidStage", "Rematch: Lord of Lightning" },
    { "RevengeMofumofuStage", "Rematch: Mechawiggler" },
    { "RevengeGiantWanderBossStage", "Rematch: Mollusque" },
    { "RevengeForestBossStage", "Rematch: Torkdrift" },
    { "YoshiCloudExStage", "Yoshi Sea of Clouds" },
};
inline constexpr const char* sSubAreasPeachNames[] = {
    "8-Bit Bullet Bills",
    "Castle Courtyard 64",
    "Crazy Cap Store",
    "Painting: Cookatiel",
    "Painting: Knucklotec",
    "Painting: Lord of Lightning",
    "Painting: Mechawiggler",
    "Painting: Mollusque",
    "Painting: Torkdrift",
    "Peach's Castle",
    "Picture Match (Mario)",
    "Rematch: Cookatiel",
    "Rematch: Knucklotec",
    "Rematch: Lord of Lightning",
    "Rematch: Mechawiggler",
    "Rematch: Mollusque",
    "Rematch: Torkdrift",
    "Yoshi Sea of Clouds",
};
inline constexpr SubArea sSubAreasSpecial1[] = {
    { "KillerRoadNoCapExStage", "Breakdown Road (No Cap)" },
    { "PackunPoisonNoCapExStage", "Invisible Road (No Cap)" },
    { "Special1WorldTowerBombTailStage", "Rabbit Ridge: Hariet" },
    { "Special1WorldTowerCapThrowerStage", "Rabbit Ridge: Rango" },
    { "Special1WorldTowerFireBlowerStage", "Rabbit Ridge: Spewart" },
    { "Special1WorldTowerStackerStage", "Rabbit Ridge: Topper" },
    { "BikeSteelNoCapExStage", "Vanishing Road" },
    { "SenobiTowerYoshiExStage", "Yoshi Sinking Island" },
    { "ShootingCityYoshiExStage", "Yoshi Under Siege" },
    { "LavaWorldUpDownYoshiExStage", "Yoshi's Magma Swamp" },
};
inline constexpr const char* sSubAreasSpecial1Names[] = {
    "Breakdown Road (No Cap)",
    "Invisible Road (No Cap)",
    "Rabbit Ridge: Hariet",
    "Rabbit Ridge: Rango",
    "Rabbit Ridge: Spewart",
    "Rabbit Ridge: Topper",
    "Vanishing Road",
    "Yoshi Sinking Island",
    "Yoshi Under Siege",
    "Yoshi's Magma Swamp",
};
inline constexpr SubArea sSubAreasSpecial2[] = {
    { "Special2WorldKoopaStage", "Culmina Crater: Bowser" },
    { "Special2WorldLavaStage", "Culmina Crater: Inside" },
    { "Special2WorldCloudStage", "Culmina Crater: Pokio" },
};
inline constexpr const char* sSubAreasSpecial2Names[] = {
    "Culmina Crater: Bowser",
    "Culmina Crater: Inside",
    "Culmina Crater: Pokio",
};

inline constexpr KingdomSubAreas sKingdomSubAreas[] = {
    { "Cap Sub Area", sSubAreasCap, sSubAreasCapNames, 5 },
    { "Cascade Sub Area", sSubAreasWaterfall, sSubAreasWaterfallNames, 5 },
    { "Sand Sub Area", sSubAreasSand, sSubAreasSandNames, 17 },
    { "Wooded Sub Area", sSubAreasForest, sSubAreasForestNames, 14 },
    { "Lake Sub Area", sSubAreasLake, sSubAreasLakeNames, 5 },
    { "Cloud Sub Area", sSubAreasCloud, sSubAreasCloudNames, 2 },
    { "Lost Sub Area", sSubAreasClash, sSubAreasClashNames, 3 },
    { "Metro Sub Area", sSubAreasCity, sSubAreasCityNames, 17 },
    { "Snow Sub Area", sSubAreasSnow, sSubAreasSnowNames, 12 },
    { "Seaside Sub Area", sSubAreasSea, sSubAreasSeaNames, 9 },
    { "Luncheon Sub Area", sSubAreasLava, sSubAreasLavaNames, 12 },
    { "Ruined Sub Area", sSubAreasAttack, sSubAreasAttackNames, 2 },
    { "Bowser Sub Area", sSubAreasSky, sSubAreasSkyNames, 8 },
    { "Moon Sub Area", sSubAreasMoon, sSubAreasMoonNames, 10 },
    { "Mushroom Sub Area", sSubAreasPeach, sSubAreasPeachNames, 18 },
    { "Dark Side Sub Area", sSubAreasSpecial1, sSubAreasSpecial1Names, 10 },
    { "Darker Side Sub Area", sSubAreasSpecial2, sSubAreasSpecial2Names, 3 },
};

} // namespace btt
