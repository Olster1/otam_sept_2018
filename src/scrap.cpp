/This assumes groups have to be consecutive ie. have to have group 0, 1, 2, 3 can't 
                //miss any ie. 0, 2, 4
                int nextGroup = originalGroup + 1;
                LevelData *nextGroupListAt = params->levelGroups[nextGroup];
                if(!nextGroupListAt) {
                    setStartOrEndGameTransition(params, LEVEL_0, CREDITS_MODE);
                    goToNextLevel = false;
                } else {
                    nextLevel = (LevelType)nextGroupListAt->levelType;    
                    
                    EasyAssert(params->levelsData[nextLevel].groupId == nextGroup);
                    EasyAssert(nextGroup > originalGroup);
                    
                    //NOTE: Unlock the next group
                    listAt = params->levelGroups[nextGroup];
                    EasyAssert(listAt == nextGroupListAt);
                    while(listAt) {
                        if(listAt->state == LEVEL_STATE_LOCKED) {
                            //this is since we can go back and complete levels again. 
                            listAt->state = LEVEL_STATE_UNLOCKED;
                        }
                        listAt = listAt->next;
                    }
                }