typedef enum {
    TOP_LEFT_TILE,
    TOP_CENTER_TILE,
    TOP_RIGHT_TILE,
    
    CENTER_LEFT_TILE,
    CENTER_TILE,
    CENTER_RIGHT_TILE,
    
    BOTTOM_LEFT_TILE,
    BOTTOM_CENTER_TILE,
    BOTTOM_RIGHT_TILE,

    CENTER_TOP_LEFT_TILE,
    CENTER_TOP_RIGHT_TILE,
    CENTER_BOTTOM_LEFT_TILE,
    CENTER_BOTTOM_RIGHT_TILE,

    TILE_POS_COUNT

} tile_pos_type;

typedef struct  {
    tile_pos_type Type;
    s32 E[9];
} tile_type_layout;

typedef struct {
    tile_type_layout layouts[10];
    u32 count;
} TileLayouts;

void easyTile_addLayout(TileLayouts *layoutParent, s32 *values, tile_pos_type type) {
    tile_type_layout *layout = layoutParent->layouts + layoutParent->count++;

    layout->Type = type;

    for(int i = 0; i < 9; ++i) {
        layout->E[i] = values[i];
    }
                
}

static inline TileLayouts easyTile_initLayouts() {
    TileLayouts layouts = {};
    int a[9] = {0, 0, 0, 
            0, 1, 1, 
            0, 1, 0};
    easyTile_addLayout(&layouts, a, TOP_LEFT_TILE);

    int b[9] = {0, 0, 0, 
            1, 1, 1, 
            0, 1, 0};
    easyTile_addLayout(&layouts, b, TOP_CENTER_TILE);            

    int c[9] = {0, 0, 0, 
            1, 1, 0, 
            0, 1, 0};
    easyTile_addLayout(&layouts, c, TOP_RIGHT_TILE);            

    int d[9] = {0, 1, 0, 
            0, 1, 1, 
            0, 1, 0};
    easyTile_addLayout(&layouts, d, CENTER_LEFT_TILE);            

    int e[9] = {0, 1, 0, 
            1, 1, 0, 
            0, 1, 0};
    easyTile_addLayout(&layouts, e, CENTER_RIGHT_TILE);   

    int f[9] = {0, 0, 0, 
            0, 1, 0, 
            0, 0, 0};
    easyTile_addLayout(&layouts, f, CENTER_TILE);            

    int g[9] = {0, 1, 0, 
            1, 1, 1, 
            0, 1, 0};
    easyTile_addLayout(&layouts, g, CENTER_TILE);            

    int h[9] = {0, 1, 0, 
            1, 1, 0, 
            0, 0, 0};
    easyTile_addLayout(&layouts, h, BOTTOM_RIGHT_TILE);            

    int i[9] = {0, 1, 0, 
            0, 1, 1, 
            0, 0, 0};
    easyTile_addLayout(&layouts, i, BOTTOM_LEFT_TILE);      

    int j[9] = {0, 1, 0, 
            1, 1, 1, 
            0, 0, 0};
    easyTile_addLayout(&layouts, j, BOTTOM_CENTER_TILE);            

    return layouts;
    
}

static inline tile_pos_type easyTile_getTileType(TileLayouts *layouts, int *spots) {
    tile_pos_type result = TOP_LEFT_TILE;
    EasyAssert(layouts->count == 10);

    for(int i = 0; i < layouts->count; ++i) {
        tile_type_layout *layout = layouts->layouts + i;
#define IsEqual_PosTile(Y, X) layout->E[Y*3 + X] == spots[Y*3 + X]
        if(IsEqual_PosTile(0, 1) && 
           IsEqual_PosTile(2, 1) && 
           IsEqual_PosTile(1, 0) && 
           IsEqual_PosTile(1, 2)) {
            result = layout->Type;
            if(result == CENTER_TILE) {
                if(!spots[0]) {
                    result = CENTER_TOP_LEFT_TILE;
                } else if(!spots[2]) {
                    result = CENTER_TOP_RIGHT_TILE;
                } else if(!spots[6]) {
                    result = CENTER_BOTTOM_LEFT_TILE;
                } else if(!spots[8]) {
                    result = CENTER_BOTTOM_RIGHT_TILE;
                }
            }
            break;
        }
        
    }
    
    //EasyAssert(Result != NULL_TILE);
    return result;
}