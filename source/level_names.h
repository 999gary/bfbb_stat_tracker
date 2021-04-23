#ifndef LEVEL_NAMES_H
#define LEVEL_NAMES_H

const char *get_level_name(char *code) {
    struct {
        char *game_level_code;
        char *level_name;
    } level_names[] = {{"MNU3", "Main Menu"},
        {"HB00", "Intro Cutscene"},
        
        {"HB01", "Bikini Bottom"},
        {"HB02", "Spongebob House"},
        {"HB03", "Squidward House"},
        {"HB04", "Patrick House"},
        {"HB06", "Shady Shoals"},
        {"HB09", "Police Station"},
        {"HB05", "Treedome"},
        {"HB07", "Krusty Krab"},
        {"HB08", "Chum Bucket"},
        {"HB10", "Theater"},
        
        {"B101", "Poseidome"},
        {"B201", "Industrial Park"},
        
        {"JF01", "Jellyfish Rock"},
        {"JF02", "Jellyfish Caves"},
        {"JF03", "Jellyfish Lake"},
        {"JF04", "Jellyfish Mountain"},
        
        {"BB01", "Downtown Streets"},
        {"BB02", "Downtown Rooftops"},
        {"BB03", "Downtown Lighthouse"},
        {"BB04", "Downtown Sea Needle"},
        
        {"GL01", "Goo Lagoon Beach"},
        {"GL02", "Goo Lagoon Caves"},
        {"GL03", "Goo Lagoon Pier"},
        
        {"BC01", "Mermalair Entrance Area"},
        {"BC02", "Mermalair Main Chamber"},
        {"BC03", "Mermalair Security Tunnel"},
        {"BC04", "Mermalair Ballroom"},
        {"BC05", "Mermalair Villain Containment"},
        
        {"RB01", "Rock Bottom Downtown"},
        {"RB02", "Rock Bottom Museum"},
        {"RB03", "Rock Bottom Trench"},
        
        {"SM01", "Sand Mountain Hub"},
        {"SM02", "Sand Mountain Slide1"},
        {"SM03", "Sand Mountain Slide2"},
        {"SM04", "Sand Mountain Slide3"},
        
        {"KF01", "Kelp Forest"},
        {"KF02", "Kelp Forest Swamps"},
        {"KF03", "Kelp Forest Caves"},
        {"KF04", "Kelp Forest Slide"},
        
        {"GY01", "Graveyard Lake"},
        {"GY02", "Graveyard Shipwreck"},
        {"GY03", "Graveyard Ship"},
        {"GY04", "Graveyard Boss"},
        
        {"DB01", "Spongebobs Dream"},
        {"DB02", "Sandys Dream"},
        {"DB03", "Squidwards Dream"},
        {"DB04", "Krabs Dream"},
        {"DB06", "Patricks Dream"},
        
        {"B302", "Chum Bucket Lab"},
        {"B303", "Chum Bucket Brain"},
        
        {"PG12", "Spongeball Arena"} };
    
    for (s32 i = 0; i < ArrayCount(level_names); ++i) {
        if (!strcmp(level_names[i].game_level_code, code)) {
            return level_names[i].level_name;
        }
    }
    
    return NULL;
}

#endif //LEVEL_NAMES_H
