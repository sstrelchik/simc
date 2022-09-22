#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"
#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct warlock_t;

//Used for version checking in code (e.g. PTR vs Live)
enum version_check_e
{
  VERSION_PTR,
  VERSION_ANY
};

template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

struct warlock_td_t : public actor_target_data_t
{
  // Cross-spec
  propagate_const<dot_t*> dots_drain_life;
  propagate_const<dot_t*> dots_drain_life_aoe; // Affliction - Soul Rot effect
  propagate_const<dot_t*> dots_soul_rot; // DF - Affliction only
  propagate_const<dot_t*> dots_corruption; // DF - Removed from Destruction

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_siphon_life;
  propagate_const<dot_t*> dots_phantom_singularity;
  propagate_const<dot_t*> dots_unstable_affliction;
  propagate_const<dot_t*> dots_vile_taint;

  propagate_const<buff_t*> debuffs_haunt;
  propagate_const<buff_t*> debuffs_shadow_embrace;
  // DF - Malefic Affliction (debuff on target for talent)
  // DF - Dread Touch (debuff on target for talent)

  // Destro
  propagate_const<dot_t*> dots_immolate;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_eradication;
  propagate_const<buff_t*> debuffs_roaring_blaze;
  propagate_const<buff_t*> debuffs_havoc;
  // DF - Pyrogenics (Rain of Fire increases Fire damage taken)

  // Demo
  propagate_const<dot_t*> dots_doom;

  propagate_const<buff_t*> debuffs_from_the_shadows;
  // DF - Fel Sunder? (Felstorm increases target's damage taken; could be in pet module instead)

  double soc_threshold; //Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    soc_threshold = 0;
  }

  void target_demise();

  int count_affliction_dots();
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; // Used for handling Unstable Affliction target swaps
  std::vector<action_t*> havoc_spells;  // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  std::vector<event_t*> wild_imp_spawns;      // Used for tracking incoming imps from HoG

  unsigned active_pets;

  // This should hold any spell data that is guaranteed in the base class or spec, without talents or other external systems required
  struct base_t
  {
    // Shared
    const spell_data_t* drain_life;
    const spell_data_t* corruption;
    const spell_data_t* shadow_bolt;
    const spell_data_t* nethermancy; // Int bonus for all cloth slots. TOCHECK: As of 2022-09-21 this is possibly bugged on beta and not working

    // Affliction
    const spell_data_t* agony;
    const spell_data_t* agony_2; // Rank 2 still learned on level up, grants increased max stacks
    const spell_data_t* potent_afflictions; // Affliction Mastery - Increased DoT and Malefic Rapture damage
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* demonic_core; // The passive responsible for the proc chance
    const spell_data_t* demonic_core_buff; // Buff spell data
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* demonology_warlock; // Spec aura

    // Destruction
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* incinerate_energize; // Soul Shard data is in a separate spell
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* destruction_warlock; // Spec aura
  } warlock_base;

  // Main pet held in active/last, guardians should be handled by pet spawners. TODO: Use spawner for Infernal/Darkglare?
  struct pets_t
  {
    warlock_pet_t* active;
    warlock_pet_t* last;
    static const int INFERNAL_LIMIT  = 1;
    static const int DARKGLARE_LIMIT = 1;

    //TODO: Refactor infernal code including new talent Rain of Chaos
    std::array<pets::destruction::infernal_t*, INFERNAL_LIMIT> infernals;
    spawner::pet_spawner_t<pets::destruction::infernal_t, warlock_t>
        roc_infernals;  // Infernal(s) summoned by Rain of Chaos
    spawner::pet_spawner_t<pets::destruction::blasphemy_t, warlock_t>
        blasphemy;  // DF - Now a Destruction Talent

    std::array<pets::affliction::darkglare_t*, DARKGLARE_LIMIT> darkglare;

    spawner::pet_spawner_t<pets::demonology::dreadstalker_t, warlock_t> dreadstalkers;
    spawner::pet_spawner_t<pets::demonology::vilefiend_t, warlock_t> vilefiends;
    spawner::pet_spawner_t<pets::demonology::demonic_tyrant_t, warlock_t> demonic_tyrants;
    spawner::pet_spawner_t<pets::demonology::grimoire_felguard_pet_t, warlock_t> grimoire_felguards;

    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;
    // DF - New Wild Imp variant - Imp Gang Boss


    // DF - Soulkeeper and Inquisitor Eye are not guardians (Bilescourge Bombers/Arcane Familiar are more appropriate matches, respectively)

    // DF - Nether Portal demons - check spawn rates
    spawner::pet_spawner_t<pets::demonology::random_demons::shivarra_t, warlock_t> shivarra;
    spawner::pet_spawner_t<pets::demonology::random_demons::darkhound_t, warlock_t> darkhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::bilescourge_t, warlock_t> bilescourges;
    spawner::pet_spawner_t<pets::demonology::random_demons::urzul_t, warlock_t> urzuls;
    spawner::pet_spawner_t<pets::demonology::random_demons::void_terror_t, warlock_t> void_terrors;
    spawner::pet_spawner_t<pets::demonology::random_demons::wrathguard_t, warlock_t> wrathguards;
    spawner::pet_spawner_t<pets::demonology::random_demons::vicious_hellhound_t, warlock_t> vicious_hellhounds;
    spawner::pet_spawner_t<pets::demonology::random_demons::illidari_satyr_t, warlock_t> illidari_satyrs;
    spawner::pet_spawner_t<pets::demonology::random_demons::eyes_of_guldan_t, warlock_t> eyes_of_guldan;
    spawner::pet_spawner_t<pets::demonology::random_demons::prince_malchezaar_t, warlock_t> prince_malchezaar;

    // DF - New post-NP summon: Pit Lord

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  //TODO: DF - Rename this section and leverage for some common purpose like 
  struct active_t
  {
    spell_t* rain_of_fire; //TODO: DF - This is the definition for the ground aoe event, how is it used?
    spell_t* bilescourge_bombers; //TODO: DF - This is the definition for the ground aoe event, how is it used?
    spell_t* summon_random_demon; //TODO: DF - This is the definition for a helper action for Nether Portal, does it belong here?
  } active;

  // DF - Does everything go in this struct? Probably yes, though spell_data_t could be replaced with player_talent_t
  // Talents
  struct talents_t
  {
    // Shared
    const spell_data_t* grimoire_of_sacrifice; // DF - Should be unchanged, but verify spec-based limitation (Aff/Destro only)

    // Class Tree
    const spell_data_t* soul_conduit; // DF - Verify unchanged other than in class tree now
    // DF - Demonic Embrace is a stamina talent, may be irrelevant now
    // DF - Demonic Inspiration (Pet haste on soul shard fill)
    // DF - Wrathful Minion (Pet damage on soul shard fill)
    // DF - Demonic Fortitude is a health talent, may be irrelevant now
    // DF - Grimoire of Synergy (moved from SL Legendary power)
    // DF - Claw of Endereth (moved from SL Legendary power)
    // DF - Summon Soulkeeper (Active ground aoe which spends hidden stacking buff)
    // DF - Inquisitor's Gaze (Non-guardian pet summon which behaves like Arcane Familiar)

    // AFF
    player_talent_t malefic_rapture;
    const spell_data_t* malefic_rapture_dmg; // Damage events use this ID, but primary talent contains the spcoeff

    // DF - Unstable Affliction
    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise, still seems to be separate spell (auto-learned on spec switch?)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration, still seems to be separate spell (auto-learned on spec switch?)
    // DF - Seed of Corruption

    const spell_data_t* nightfall; //TODO: RNG information is missing from spell data, and data also says buff can potentially stack to 2. Serious testing needed, especially with multiple corruptions out!
    // DF - Xavian Teachings (Formerly Corruption Rank 2 + 3)
    const spell_data_t* sow_the_seeds;

    const spell_data_t* shadow_embrace; // DF - Now a 2 point talent
    // DF - Harvester of Souls (2 point talent, instant damage proc chance on Corruption ticks)
    const spell_data_t* writhe_in_agony; // DF - Now a 2 point talent
    // DF - Agonizing Corruption (2 point talent, Seed of Corruption applies Agony stacks)

    const spell_data_t* drain_soul; // DF - Unchanged, still replaces Shadow Bolt
    const spell_data_t* absolute_corruption; // DF - Now a choice against Siphon Life
    const spell_data_t* siphon_life; // DF - Now a choice against Absolute Corruption
    const spell_data_t* phantom_singularity; // DF - Now a choice against Vile Taint, TODO: Dot duration uses hardcoded tick count see if spell data available
    const spell_data_t* vile_taint; // DF - New functionality (applies Agony on targets hit)

    // DF - Soul Tap (Sacrifice Soul Leech for Soul Shard)
    const spell_data_t* inevitable_demise; // DF - Now a 2 point talent
    // DF - Soul Swap (Spend Soul Shard to apply core dots)
    // DF - Soul Flame (2 point talent, AoE damage on kills)
    // Grimoire of Sacrifice (shared with Destruction)
    
    // DF - Pandemic Invocation (2 point talent, late DoT refresh deals damage and has Soul Shard chance)
    // DF - Withering Bolt (2 point talent, moved from SL Conduit)
    // DF - Sacrolash's Dark Strike (2 point talent, moved from SL Legendary)

    const spell_data_t* creeping_death; // DF - No long reduces duration
    const spell_data_t* haunt;
    // DF - Summon Darkglare 
    // DF - Soul Rot (formerly SL Covenant Ability, now Affliction only)

    // DF - Malefic Affliction (2 point talent, stacking damage increase to current Unstable Affliction)
    // DF - Calamitous Crescendo (Formerly SL Tier Bonus)
    // DF - Seized Vitality (2 point talent, additional Haunt damage)
    // DF - Antoran Plating (2 point talent, increased Darkglare damage and duration)
    // DF - Wrath of Consumption (Formerly SL Legendary)
    // DF - Soul Eater's Gluttony (2 point talent, Soul Rot CDR from Unstable Affliction)

    // DF - Doom Blossom (Choice against Dread Touch, damage proc on Corruption ticks based on Malefic Affliction)
    // DF - Dread Touch (Choice against Doom Blossom, increased DoT damage based on Malefic Affliction)
    // DF - Haunted Soul (Haunt increase ALL DoT damage while active)
    // DF - Wilfred's Sigil of Superior Summoning (Choice against Grim Reach, formerly SL Legendary, NOTE: SHARES NAME WITH OTHER SPEC TALENTS)
    // DF - Grim Reach (Choice against Wilfred's, Darkglare hits all targets affected by DoTs)
    // DF - Decaying Soul Satchel (Formerly SL Legendary)

    // DEMO
    // DF - Call Dreadstalkers (2 dogs, check if leap included)

    // DF - Demonbolt (Demonic Core is a guaranteed passive, even though this talent is optional)
    const spell_data_t* dreadlash;
    // DF - Fel Commando (Formerly SL Conduit)

    // DF - Borne of Blood (Formerly SL Conduit)
    const spell_data_t* summon_vilefiend; // DF - Now a choice against Soul Strike
    const spell_data_t* soul_strike; // DF - Now a choice against Summon Vilefiend
    const spell_data_t* bilescourge_bombers; // DF - Now a choice against Demonic Strength
    const spell_data_t* demonic_strength; // DF - Now a choice against Bilescourge Bombers
    const spell_data_t* from_the_shadows; // DF - Should be unchanged but due to Shadowflame effect, new abilities need checking against it

    // DF - Implosion (presumably unchanged, double-check Wild Imp flight behavior)
    // DF - Shadow's Bite (Demonbolt damage increase after Dreadstalkers despawn)
    // DF - Carnivorous Stalkers (Formerly SL Conduit)
    // DF - Fel and Steel (Felstorm and Dreadbite damage increase)
    // DF - Fel Might (Shorter Felstorm CD)

    const spell_data_t* power_siphon;
    const spell_data_t* inner_demons; // DF - Now a 2 point talent
    const spell_data_t* demonic_calling; // DF - Now a 2 point talent
    const spell_data_t* grimoire_felguard;

    // DF - Bloodbound Imps (Increased Demonic Core proc chance from Wild Imps)
    // DF - Grim Inquisitor's Dread Calling (Formerly SL Legendary)
    const spell_data_t* doom; // DF - Base functionality presumably unchanged but requires double checking due to new talents
    // DF - Demonic Meteor (Chance to proc Hand of Gul'dan off Hand of Gul'dan)
    // DF - Fel Sunder (Increase damage taken debuff when hit by Felstorm)    

    // DF - Balespider's Burning Core (Formerly SL Legendary, now a 2 point talent)
    // DF - Imp Gang Boss (2 point talent, Wild Imp has chance to be this pet instead)
    // DF - Kazaak's Final Curse (2 point talent, Doom deals increased damage based on active demon count)
    // DF - Ripped Through the Portal (Formerly SL Tier Bonus, now a 2 point talent)
    // DF - Houndmaster's Gambit (2 point talent, Shadow Bolt and Demonbolt have a chance to reset Call Dreadstalkers)
    
    const spell_data_t* nether_portal; // DF - Cooldown may be reverted, will require constant checking until launch
    // DF - Summon Demonic Tyrant - Presumably unchanged, but new talents require checking against this summon
    // DF - Antoran Armaments (Increased Felguard damage and Soul Strike cleave)

    // DF - Ner'zhul's Volition (2 point talent, chance to summon additional demons from Nether Portal)
    // DF - Stolen Power (Stacking buff from Wild Imps, at max get increased Shadow Bolt or Demonbolt damage)
    const spell_data_t* sacrificed_souls; // DF - Now a 2 point talent, needs checking against new pets for compatibility
    // DF - Soulbound Tyrant (2 point talent, formerly a Rank 2 effect on Summon Demonic Tyrant)
    // DF - Forces of the Horned Nightmare (Formerly SL Legendary, now a 2 point talent)
    // DF - The Expendables (Per-pet stacking buff to damage when a Wild Imp expires)
    // DF - Command Aura (2 point talent, increased Wild Imp and Dreadstalker damage while Felguard active)

    // DF - Gul'dan's Ambition (Summon Pit Lord at end of Nether Portal)
    const spell_data_t* demonic_consumption; // DF - HEAVILY REWORKED, RENAMED "Reign of Tyranny", now a choice against Wilfred's
    // DF - Wilfred's Sigil of Superior Summoning (Choice against Reign of Tyranny, formerly SL Legendary, NOTE: SHARES NAME WITH OTHER SPEC TALENTS)
    // DF - Guillotine (Felguard AoE plus autoattack cleave cooldown)

    // DESTRO
    // DF - Chaos Bolt

    // DF - Conflagrate (base 2 charges)
    const spell_data_t* reverse_entropy; // DF - Now a choice against Internal Combustion, make sure to find correct RPPM location in data with new talent structure
    const spell_data_t* internal_combustion; // DF - Now a choice against Reverse Entropy
    // DF - Rain of Fire

    // DF - Backdraft (max 2 stacks, also now affects Soul Fire)
    // DF - Mayhem (Choice against Havoc, single target spells have a chance to cleave)
    // DF - Havoc (Choice against Mayhem, core functionality unchanged)
    // DF - Pyrogenics (Enemies affected by Rain of Fire take increased Fire damage)

    const spell_data_t* roaring_blaze; // DF - Now a choice against Improved Conflagrate
    // DF - Improved Conflagrate (Choice against Roaring Blaze, 1 additional charge for Conflagrate)
    // DF - Explosive Potential (Reduces Conflagrate cooldown)
    const spell_data_t* channel_demonfire;
    // DF - Pandemonium (Choice against Cry Havoc, additional effects for Mayhem/Havoc)
    // DF - Cry Havoc (Choice against Pandemonium, duplicated Chaos Bolts deal AoE damage)
    // DF - Improved Immolate (2 point talent, duration increase)
    const spell_data_t* inferno; // DF - Now a choice against Cataclysm, check that target cap nerfs remain
    const spell_data_t* cataclysm; // DF - Now a choice against Inferno

    const spell_data_t* soul_fire;
    const spell_data_t* shadowburn;
    // DF - Raging Demonfire (2 point talent, additional Demonfire bolts and bolts extend Immolate)
    // DF - Rolling Havoc (2 point talent, increased damage buff when spells are duplicated by Mayhem/Havoc)
    // DF - Backlash (Crit chance increase, damage proc when physically attacked)
    const spell_data_t* fire_and_brimstone; // DF - Now a 2 point talent

    // DF - Decimation (Incinerate and Conflagrate casts reduce Soul Fire cooldown)
    // DF - Conflagration of Chaos (2 point talent, Conflagrate/Shadowburn has chance to make next cast of it a guaranteed crit)
    // DF - Flashpoint (2 point talent, stacking haste buff from Immolate ticks on high-health targets)
    // DF - Scalding Flames (2 point talent, increased Immolate damage)

    // DF - Ruin (2 point talent, damage increase to several spells)
    const spell_data_t* eradication; // DF - Now a 2 point talent
    // DF - Ashen Remains (2 point talent, formerly SL Conduit)
    // Grimoire of Sacrifice (shared with Affliction)

    // DF - Summon Infernal
    // DF - Embers of the Diabolic (formerly SL Legendary)
    // DF - Ritual of Ruin (formerly SL Tier Bonus, functionality slightly modified)
    
    // DF - Crashing Chaos (2 point talent, Summon Infernal reduces cost of next X casts)
    // DF - Infernal Brand (2 point talent, formerly SL Conduit)
    // DF - Power Overwhelming (2 point talent, stacking mastery buff for spending Soul Shards)
    // DF - Madness of the Azj'aqir (2 point talent, formerly SL Legendary, now applies to more spells)
    // DF - Master Ritualist (2 point talent, reduces proc cost of Ritual of Ruin)
    // DF - Burn to Ashes (2 point talent, Chaos Bolt and Rain of Fire increase damage of next 2 Incinerates)

    const spell_data_t* rain_of_chaos; // DF - Now a choice against Wilfred's, check deck of cards RNG
    // DF - Wilfred's Sigil of Superior Summoning (Choice against Rain of Chaos, formerly SL Legendary, NOTE: SHARES NAME WITH OTHER SPEC TALENTS)
    // DF - Chaos Incarnate (Choice against Dimensional Rift, maximum mastery value for some spells)
    // DF - Dimensional Rift (Choice against Chaos Incarnate, charge cooldown instant spell which deals damage and grants fragments)
    // DF - Avatar of Destruction (Formerly SL Tier Bonus, summons Blasphemy when consuming Ritual of Ruin)
  } talents;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct legendary_t
  {
    // Legendaries
    // Cross-spec
    item_runeforge_t claw_of_endereth; // DF - Now class talent
    item_runeforge_t relic_of_demonic_synergy; // DF - Now class talent
    item_runeforge_t wilfreds_sigil_of_superior_summoning; // DF - Now a talent in all 3 spec trees
    // Affliction
    item_runeforge_t sacrolashs_dark_strike; // DF - Now an Affliction talent
    item_runeforge_t wrath_of_consumption; // DF - Now an Affliction talent
    // Demonology
    item_runeforge_t balespiders_burning_core; // DF - Now a Demonology talent
    item_runeforge_t forces_of_the_horned_nightmare; // DF - Now a Demonology talent
    item_runeforge_t grim_inquisitors_dread_calling; // DF - Now a Demonology talent
    // Destruction
    item_runeforge_t cinders_of_the_azjaqir; // DF - Reworked into Improved Conflagrate
    item_runeforge_t embers_of_the_diabolic_raiment; // DF - Now a Destruction talent
    item_runeforge_t madness_of_the_azjaqir; // DF - Now a Destruction talent
    // Covenant
    item_runeforge_t decaying_soul_satchel; // DF - Now an Affliction talent
  } legendary;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct conduit_t
  {
    // Conduits
    // Affliction
    conduit_data_t withering_bolt; // DF - Now an Affliction talent
    // Demonology
    conduit_data_t borne_of_blood; // DF - Now a Demonology talent
    conduit_data_t carnivorous_stalkers; // DF - Now a Demonology talent
    conduit_data_t fel_commando; // DF - Now a Demonology talent
    // Destruction
    conduit_data_t ashen_remains; // DF - Now a Destruction talent
    conduit_data_t infernal_brand; // DF - Now a Destruction talent
  } conduit;

  // DF - This struct will be retired, need to determine if needed for pre-patch
  struct covenant_t
  {
    // Covenant Abilities
    const spell_data_t* soul_rot;               // DF - Now an Affliction talent
  } covenant;

  // DF - To review while implementing talents for new additions
  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> phantom_singularity;
    propagate_const<cooldown_t*> darkglare;
    propagate_const<cooldown_t*> demonic_tyrant;
    propagate_const<cooldown_t*> infernal;
    propagate_const<cooldown_t*> shadowburn;
  } cooldowns;

  // DF - Retire this section, combine remnants with the mastery_spells struct above in a "core" or "base" spells section
  struct specs_t
  {
    // Affliction only
    const spell_data_t* corruption_2; // DF - Baked into Xavian Teachings talent
    const spell_data_t* corruption_3; // DF - Baked into Xavian Teachings talent
    const spell_data_t* summon_darkglare; // DF - Now an Affliction talent
    const spell_data_t* summon_darkglare_2; // DF - Baked into Affliction talent (2 minute cooldown)

    // Demonology only
    const spell_data_t* call_dreadstalkers_2; // DF - Partially baked in to Demonology talent (Cast time reduction REMOVED, leap ability retained)
    const spell_data_t* fel_firebolt_2; // DF - Baked into base Wild Imp behavior (Fel Firebolt energy cost reduction of 20%)
    const spell_data_t* summon_demonic_tyrant_2; // DF - Baked into Soulbound Tyrant talent

    // Destruction only
    const spell_data_t* conflagrate; // DF - Now a Destruction talent (base 2 charges)
    const spell_data_t* conflagrate_2; // DF - Baked into Conflagrate talent (used to be 1->2 charges)
    const spell_data_t* havoc; // DF - Now a Destruction talent
    const spell_data_t* havoc_2; // DF - Baked into Havoc talent (12 second total duration)
    const spell_data_t* rain_of_fire_2; // DF - Should be irrelevant now (used to be increased Rain of Fire damage)
    const spell_data_t* summon_infernal_2; // DF - Should be irrelevant now (used to be increased impact damage)
  } spec;

  // DF - Many new effects to be added here as talents are implemented
  // Buffs
  struct buffs_t
  {
    propagate_const<buff_t*> demonic_power; //Buff from Summon Demonic Tyrant (increased demon damage + duration)
    propagate_const<buff_t*> grimoire_of_sacrifice; //Buff which grants damage proc
    // DF - Summon Soulkeeper has a hidden stacking buff
    // DF - Determine if dummy buff should be added for Inquisitor's Eye
    propagate_const<buff_t*> demonic_synergy; // DF - Now comes from Class talent

    // Affliction Buffs
    propagate_const<buff_t*> drain_life; //Dummy buff used internally for handling Inevitable Demise cases
    propagate_const<buff_t*> nightfall;
    propagate_const<buff_t*> inevitable_demise;
    propagate_const<buff_t*> calamitous_crescendo;
    propagate_const<buff_t*> soul_rot; // DF - Now Affliction only. Buff for determining if Drain Life is zero cost and aoe.
    propagate_const<buff_t*> wrath_of_consumption; // DF - Now comes from Affliction talent.
    propagate_const<buff_t*> decaying_soul_satchel_haste; // DF - Now comes from Affliction talent
    propagate_const<buff_t*> decaying_soul_satchel_crit; // These are one unified buff in-game but splitting them in simc to make it easier to apply stat pcts
    // DF - Haunted Soul - Buff on player while active

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> power_siphon; //Hidden buff from Power Siphon that increases damage of successive Demonbolts
    propagate_const<buff_t*> demonic_calling;
    propagate_const<buff_t*> inner_demons;
    propagate_const<buff_t*> nether_portal;
    propagate_const<buff_t*> wild_imps; //Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)
    propagate_const<buff_t*> dreadstalkers; //Buff for tracking number of Dreadstalkers currently out
    propagate_const<buff_t*> vilefiend; //Buff for tracking if Vilefiend is currently out
    propagate_const<buff_t*> tyrant; //Buff for tracking if Demonic Tyrant is currently out
    propagate_const<buff_t*> portal_summons; // DF - This dummy buff may be unused for any practical purpose and could be removed
    propagate_const<buff_t*> grimoire_felguard; //Buff for tracking if GFG pet is currently out
    propagate_const<buff_t*> prince_malchezaar; //Buff for tracking Malchezaar (who is currently disabled in sims)
    propagate_const<buff_t*> eyes_of_guldan; //Buff for tracking if rare random summon is currently out
    propagate_const<buff_t*> dread_calling; // DF - Now comes from Demonology talent
    propagate_const<buff_t*> balespiders_burning_core; // DF - Now comes from Demonology talent
    // DF - Shadow's Bite (Demonbolt damage buff on player)
    // DF - Stolen Power (stacking buff from Wild Imp casts) - Could actually be two buffs, currently broken

    // Destruction Buffs
    propagate_const<buff_t*> backdraft; // DF - Max 2 stacks
    propagate_const<buff_t*> reverse_entropy;
    propagate_const<buff_t*> rain_of_chaos;
    propagate_const<buff_t*> impending_ruin; // DF - Impending Ruin and Ritual of Ruin now come from Destruction talent
    propagate_const<buff_t*> ritual_of_ruin;
    propagate_const<buff_t*> madness_of_the_azjaqir; // DF - Now comes from Destruction talent
    // DF - Mayhem could be a hidden aura, if not a dummy buff could possibly simplify implementation anyway
    // DF - Rolling Havoc (stacking damage increase when Mayhem/Havoc cleaves)
    // DF - Backlash? (passive crit increase)
    // DF - Flashpoint (stacking haste from Immolate ticks)
    // DF - Crashing Chaos (cost reduction after Infernal summon)
    // DF - Power Overwhelming (stacking mastery when spending Soul Shards)
    // DF - Burn to Ashes (increased Incinerate damage after Chaos Bolt/Rain of Fire)
    // DF - Chaos Incarnate? (passive max mastery on certain spells)
  } buffs;

  //TODO: Determine if any gains are not currently being tracked
  // Gains - Many of these are automatically handled for resource gains if get_gain( name ) is given the same name as the action source
  struct gains_t
  {
    gain_t* soul_conduit;

    gain_t* agony;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;

    gain_t* conflagrate;
    gain_t* incinerate;
    gain_t* incinerate_crits;
    gain_t* incinerate_fnb;
    gain_t* incinerate_fnb_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* soul_fire;
    gain_t* infernal;
    gain_t* shadowburn_refund;
    gain_t* inferno;

    gain_t* miss_refund;

    gain_t* shadow_bolt;
    gain_t* doom;
    gain_t* summon_demonic_tyrant;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* soul_conduit;

    // aff
    proc_t* nightfall;
    proc_t* calamitous_crescendo;
    std::array<proc_t*, 8> malefic_rapture; // This length should be at least equal to the maximum number of Affliction DoTs that can be active on a target.

    // demo
    proc_t* demonic_calling;
    proc_t* one_shard_hog;
    proc_t* two_shard_hog;
    proc_t* three_shard_hog;
    proc_t* summon_random_demon;
    proc_t* portal_summon;
    proc_t* carnivorous_stalkers; // DF - Now a Demonology talent
    proc_t* horned_nightmare; // DF - Now a Demonology talent

    // destro
    proc_t* reverse_entropy;
    proc_t* rain_of_chaos;
    proc_t* ritual_of_ruin;
    proc_t* avatar_of_destruction;
  } procs;

  int initial_soul_shards;
  std::string default_pet;
  shuffled_rng_t* rain_of_chaos_rng;
  // DF - Possibly add pre-patch spell for automatic switchover for DF

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  int get_spawning_imp_count();
  timespan_t time_to_imps( int count );
  int imps_spawned_during( timespan_t period );
  void darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension );
  bool min_version_check( version_check_e version ) const;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override
  {
    return ROLE_SPELL;
  }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_haste() const override;
  double composite_melee_haste() const override;
  double composite_mastery() const override;
  double resource_regen_per_second( resource_e ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override { return warlock_apl::potion( this ); }
  std::string default_flask() const override { return warlock_apl::flask( this ); }
  std::string default_food() const override { return warlock_apl::food( this ); }
  std::string default_rune() const override { return warlock_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return warlock_apl::temporary_enchant( this ); }
  void apply_affecting_auras( action_t& action ) override;

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  // sc_warlock
  action_t* create_action_warlock( util::string_view, util::string_view );

  // sc_warlock_affliction
  action_t* create_action_affliction( util::string_view, util::string_view );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();

  // sc_warlock_demonology
  action_t* create_action_demonology( util::string_view, util::string_view );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();

  // sc_warlock_destruction
  action_t* create_action_destruction( util::string_view, util::string_view );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();

  // sc_warlock_pets
  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  pet_t* create_demo_pet( util::string_view pet_name, util::string_view pet_type );
  void create_all_pets();
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
};

namespace actions
{
//Event for triggering delayed refunds from Soul Conduit
//Delay prevents instant reaction time issues for rng refunds
struct sc_event_t : public player_event_t
{
  gain_t* shard_gain;
  warlock_t* pl;
  int shards_used;

  sc_event_t( warlock_t* p, int c )
    : player_event_t( *p, 100_ms ),
    shard_gain( p->gains.soul_conduit ),
    pl( p ),
    shards_used( c )
  {
  }

  virtual const char* name() const override
  {
    return "soul_conduit_event";
  }

  virtual void execute() override
  {
    double soul_conduit_rng = pl->talents.soul_conduit->effectN( 1 ).percent();

    for ( int i = 0; i < shards_used; i++ )
    {
      if ( rng().roll( soul_conduit_rng ) )
      {
        pl->sim->print_log( "Soul Conduit proc occurred for Warlock {}, refunding 1.0 soul shards.", pl->name() );
        pl->resource_gain( RESOURCE_SOUL_SHARD, 1.0, shard_gain );
        pl->procs.soul_conduit->occur();
      }
    }
  }
};

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( util::string_view n, warlock_t* p, const uint32_t id ) : heal_t( n, p, p->find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }
};

struct warlock_spell_t : public spell_t
{
public:
  gain_t* gain;
  bool can_havoc; // DF - Also need to utilize this for Mayhem
  bool affected_by_woc; // DF - This is now an Affliction talent, see if this hardcoded bool is still needed

  warlock_spell_t( warlock_t* p, util::string_view n ) : warlock_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  warlock_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : warlock_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
    can_havoc         = false;

    //TOCHECK: Is there a way to link this to the buffs.x spell data so we don't have to remember this is hardcoded?
    affected_by_woc   = data().affected_by( p->find_spell( 337130 )->effectN( 1 ) );
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t )
  {
    return p()->get_target_data( t );
  }

  const warlock_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  void reset() override
  {
    spell_t::reset();
  }

  double cost() const override
  {
    double c = spell_t::cost();
    return c;
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      // lets try making all lock specs not react instantly to shard gen
      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( last_resource_cost ) );
      }

      if ( p()->legendary.wilfreds_sigil_of_superior_summoning->ok() )
      {
        switch ( p()->specialization() )
        {
          case WARLOCK_AFFLICTION:
            p()->cooldowns.darkglare->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 1 ).time_value(), false );
            break;
          case WARLOCK_DEMONOLOGY:
            p()->cooldowns.demonic_tyrant->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 2 ).time_value(), false );
            break;
          case WARLOCK_DESTRUCTION:
            p()->cooldowns.infernal->adjust( -last_resource_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 3 ).time_value(), false );
            break;
          default:
            break;
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = spell_t::composite_target_multiplier( t );
    return m;
  }

  double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    pm *= 1.0 + p()->buffs.demonic_synergy->check_stack_value();

    return pm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = spell_t::composite_ta_multiplier( s );

    if ( p()->legendary.wrath_of_consumption.ok() && p()->buffs.wrath_of_consumption->check() && affected_by_woc )
      m *= 1.0 + p()->buffs.wrath_of_consumption->check_stack_value();

    return m;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot->is_ticking() )
    {
      dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
    }
  }

  //Destruction specific things for Havoc that unfortunately need to be in main module

  bool use_havoc() const
  {
    // Ensure we do not try to hit the same target twice.
    return can_havoc && p()->havoc_target && p()->havoc_target != target;
  }

  int n_targets() const override
  {
    if ( p()->specialization() == WARLOCK_DESTRUCTION && use_havoc() )
    {
      assert(spell_t::n_targets() == 0);
      return 2;
    }
    else
      return spell_t::n_targets();
  }

  size_t available_targets(std::vector<player_t*>& tl) const override
  {
    spell_t::available_targets(tl);

    // Check target list size to prevent some silly scenarios where Havoc target
    // is the only target in the list.
    if ( p()->specialization() == WARLOCK_DESTRUCTION && tl.size() > 1 && use_havoc())
    {
      // We need to make sure that the Havoc target ends up second in the target list,
      // so that Havoc spells can pick it up correctly.
      auto it = range::find(tl, p()->havoc_target);
      if (it != tl.end())
      {
        tl.erase(it);
        tl.insert(tl.begin() + 1, p()->havoc_target);
      }
    }

    return tl.size();
  }

  void init() override
  {
    spell_t::init();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && can_havoc )
    {
        // SL - Conduit
        base_aoe_multiplier *= p()->spec.havoc->effectN(1).percent();
        p()->havoc_spells.push_back(this);
    }
  }

  //End Destruction specific things

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    return spell_t::create_expression( name_str );
  }
};

struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
{
  grimoire_of_sacrifice_damage_t(warlock_t* p)
    : warlock_spell_t("grimoire_of_sacrifice_damage_proc", p, p->find_spell(196100))
  {
    background = true;
    proc = true;
  }
};

struct demonic_synergy_callback_t : public dbc_proc_callback_t
{
  warlock_t* owner;

  demonic_synergy_callback_t( warlock_t* p, special_effect_t& e )
    : dbc_proc_callback_t( p, e ), owner( p )
  {
  }

  void execute( action_t* /* a */, action_state_t* ) override
  {
    if ( owner->warlock_pet_list.active )
    {
      auto pet = owner->warlock_pet_list.active;
      //Always set the pet's buff value using the owner's to ensure specialization value is correct
      pet->buffs.demonic_synergy->trigger( 1, owner->buffs.demonic_synergy->default_value );
    }
  }
};

using residual_action_t = residual_action::residual_periodic_action_t<warlock_spell_t>;

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) == p()->pet_name_list.end() )
    {
      p()->pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( util::string_view n, warlock_t* p, util::string_view sname = {} )
    : warlock_spell_t( p, sname.empty() ? fmt::format( "Summon {}", n ) : sname ),
      summoning_duration( timespan_t::zero() ),
      pet_name( sname.empty() ? n : sname ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( util::string_view n, warlock_t* p, int id )
    : warlock_spell_t( n, p, p->find_spell( id ) ),
      summoning_duration( timespan_t::zero() ),
      pet_name( n ),
      pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( util::string_view n, warlock_t* p, const spell_data_t* sd )
    : warlock_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  void init_finished() override
  {
    pet = debug_cast<warlock_pet_t*>( player->find_pet( pet_name ) );

    warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet->summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }
    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t : public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( util::string_view n, warlock_t* p, int id )
    : summon_pet_t( n, p, id ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = 60_s;
    ignore_false_positive      = true;
  }

  summon_main_pet_t( util::string_view n, warlock_t* p )
    : summon_pet_t( n, p ), instant_cooldown( p->get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown->duration = 60_s;
    ignore_false_positive      = true;
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p()->warlock_pet_list.active )
    {
      p()->warlock_pet_list.active->dismiss();
      p()->warlock_pet_list.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p()->warlock_pet_list.active == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p()->warlock_pet_list.active = p()->warlock_pet_list.last = pet;

    if ( p()->buffs.grimoire_of_sacrifice->check() )
      p()->buffs.grimoire_of_sacrifice->expire();
  }
};

// Event for spawning wild imps for Demonology
// Placed in warlock.cpp for expression purposes
struct imp_delay_event_t : public player_event_t
{
  timespan_t diff;

  imp_delay_event_t( warlock_t* p, double delay, double exp ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  {
    diff = timespan_t::from_millis( exp - delay );
  }

  virtual const char* name() const override
  {
    return "imp_delay";
  }

  virtual void execute() override
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    p->warlock_pet_list.wild_imps.spawn();

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t expected_time()
  {
    return std::max( 0_ms, this->remains() + diff );
  }
};
}  // namespace actions

namespace buffs
{
template <typename Base>
struct warlock_buff_t : public Base
{
public:
  using base_t = warlock_buff_t;
  warlock_buff_t( warlock_td_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( p, name, s, item )
  {
  }

  warlock_buff_t( warlock_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                  const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  warlock_t* p()
  {
    return static_cast<warlock_t*>( Base::source );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( Base::source );
  }
};
}  // namespace buffs


}  // namespace warlock
