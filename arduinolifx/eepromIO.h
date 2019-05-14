void writeEepromDefaults() {
  // first time sketch has been run, set defaults into EEPROM
  debug_println(F("Config does not exist in EEPROM, writing..."));

  EEPROM.write(EEPROM_CONFIG_START, EEPROM_CONFIG[0]);
  EEPROM.write(EEPROM_CONFIG_START + 1, EEPROM_CONFIG[1]);
  EEPROM.write(EEPROM_CONFIG_START + 2, EEPROM_CONFIG[2]);

  for (int i = 0; i < LifxBulbLabelLength; i++) {
    EEPROM.write(EEPROM_BULB_LABEL_START + i, bulbLabel[i]);
  }
  for (int i = 0; i < LifxBulbTagsLength; i++) {
    EEPROM.write(EEPROM_BULB_TAGS_START + i, bulbTags[i]);
  }
  for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
    EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, bulbTagLabels[i]);
  }
  for (int i = 0; i < LifxBulbLocationIDLength; i++) {
    EEPROM.write(EEPROM_LOCATION_START + i, location[i]);
  }
  for (int i = 0; i < LifxBulbLocationLabelLength; i++) {
    EEPROM.write(EEPROM_LOCATION_START + LifxBulbLocationIDLength + i, bulbLocationLabel[i]);
  }
  for (int i = 0; i < LifxBulbLocationUpdatedAtLength; i++) {
    EEPROM.write(EEPROM_LOCATION_START + LifxBulbLocationIDLength + LifxBulbLocationLabelLength + i, loc_updatedAt[i]);
  }
  for (int i = 0; i < LifxBulbGroupIDLength; i++) {
    EEPROM.write(EEPROM_GROUP_START + i, group[i]);
  }
  for (int i = 0; i < LifxBulbGroupLabelLength; i++) {
    EEPROM.write(EEPROM_GROUP_START + LifxBulbGroupIDLength + i, bulbGroupLabel[i]);
  }
  for (int i = 0; i < LifxBulbGroupUpdatedAtLength; i++) {
    EEPROM.write(EEPROM_GROUP_START + LifxBulbGroupIDLength + LifxBulbGroupLabelLength + i, group_updatedAt[i]);
  }
  EEPROM.commit();
  debug_println(F("Done writing EEPROM config."));
}

bool eepromExists() {
  // read in settings from EEPROM (if they exist) for bulb label and tags
  if (EEPROM.read(EEPROM_CONFIG_START) == EEPROM_CONFIG[0]
      && EEPROM.read(EEPROM_CONFIG_START + 1) == EEPROM_CONFIG[1]
      && EEPROM.read(EEPROM_CONFIG_START + 2) == EEPROM_CONFIG[2]) {
    debug_println(F("Config exists in EEPROM, reading..."));
    return true;
  } else {
    return false;
  }
}

void readEepromDefaults() {
  //bulb label
  debug_print(F("Bulb label: "));
  for (int i = 0; i < LifxBulbLabelLength; i++) {
    bulbLabel[i] = EEPROM.read(EEPROM_BULB_LABEL_START + i);
    debug_print(bulbLabel[i]);
  }
  debug_println();

  //tags
  debug_print(F("Bulb tags: "));
  for (int i = 0; i < LifxBulbTagsLength; i++) {
    bulbTags[i] = EEPROM.read(EEPROM_BULB_TAGS_START + i);
    debug_print(bulbTags[i]);
  }
  debug_println();

  debug_print(F("Bulb tag labels: "));
  for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
    bulbTagLabels[i] = EEPROM.read(EEPROM_BULB_TAG_LABELS_START + i);
    debug_print(bulbTagLabels[i]);
  }
  debug_println();

  //Location
  debug_print(F("Bulb Location ID: "));
  for (int i = 0; i < LifxBulbLocationIDLength; i++) {
    location[i] = EEPROM.read(EEPROM_LOCATION_START + i);
    debug_print(location[i],HEX);
  }
  debug_println();

  debug_print(F("Bulb Location: "));
  for (int i = 0; i < LifxBulbLocationLabelLength; i++) {
    bulbLocationLabel[i] = EEPROM.read(EEPROM_LOCATION_START + LifxBulbLocationIDLength + i);
    debug_print(bulbLocationLabel[i]);
  }
  debug_println();

  debug_print(F("Bulb Location Updated at: "));
  for (int i = 0; i < LifxBulbLocationUpdatedAtLength; i++) {
    loc_updatedAt[i] = EEPROM.read(EEPROM_LOCATION_START + LifxBulbLocationIDLength + LifxBulbLocationLabelLength + i);
    debug_print(loc_updatedAt[i],HEX);
  }
  debug_println();

  //Group
  debug_print(F("Bulb Group ID: "));
  for (int i = 0; i < LifxBulbGroupIDLength; i++) {
    group[i] = EEPROM.read(EEPROM_GROUP_START + i);
    debug_print(group[i],HEX);
  }
  debug_println();

  debug_print(F("Bulb Group: "));
  for (int i = 0; i < LifxBulbGroupLabelLength; i++) {
    bulbGroupLabel[i] = EEPROM.read(EEPROM_GROUP_START + LifxBulbGroupIDLength + i);
    debug_print(bulbGroupLabel[i]);
  }
  debug_println();

  debug_print(F("Bulb Group Updated at: "));
  for (int i = 0; i < LifxBulbGroupUpdatedAtLength; i++) {
    group_updatedAt[i] = EEPROM.read(EEPROM_GROUP_START + LifxBulbGroupIDLength + LifxBulbGroupLabelLength + i);
    debug_print(group_updatedAt[i],HEX);
  }
  debug_println();
  debug_println(F("Done reading EEPROM config."));
}

void dumpEeprom() {
  debug_println(F("#############################################################################"));
  debug_println(F("EEPROM dump:"));
  for (int i = 0; i < 256; i++) {
    debug_print(EEPROM.read(i),HEX);
    debug_print(SPACE);
  }
  debug_println();
  debug_println(F("#############################################################################"));  
  debug_println();
}
void wipeEeprom() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i,0);
  }
  EEPROM.commit();
}

