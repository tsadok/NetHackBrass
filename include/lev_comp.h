typedef union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
} YYSTYPE;
#define	CHAR	258
#define	INTEGER	259
#define	BOOLEAN	260
#define	PERCENT	261
#define	MESSAGE_ID	262
#define	MAZE_ID	263
#define	LEVEL_ID	264
#define	LEV_INIT_ID	265
#define	GEOMETRY_ID	266
#define	NOMAP_ID	267
#define	OBJECT_ID	268
#define	COBJECT_ID	269
#define	MONSTER_ID	270
#define	TRAP_ID	271
#define	DOOR_ID	272
#define	DRAWBRIDGE_ID	273
#define	MAZEWALK_ID	274
#define	WALLIFY_ID	275
#define	REGION_ID	276
#define	FILLING	277
#define	RANDOM_OBJECTS_ID	278
#define	RANDOM_MONSTERS_ID	279
#define	RANDOM_PLACES_ID	280
#define	ALTAR_ID	281
#define	LADDER_ID	282
#define	STAIR_ID	283
#define	NON_DIGGABLE_ID	284
#define	NON_PASSWALL_ID	285
#define	ROOM_ID	286
#define	PORTAL_ID	287
#define	TELEPRT_ID	288
#define	BRANCH_ID	289
#define	LEV	290
#define	CHANCE_ID	291
#define	CORRIDOR_ID	292
#define	GOLD_ID	293
#define	ENGRAVING_ID	294
#define	FOUNTAIN_ID	295
#define	POOL_ID	296
#define	SINK_ID	297
#define	NONE	298
#define	RAND_CORRIDOR_ID	299
#define	DOOR_STATE	300
#define	LIGHT_STATE	301
#define	CURSE_TYPE	302
#define	ENGRAVING_TYPE	303
#define	ROOMSHAPE	304
#define	DIRECTION	305
#define	RANDOM_TYPE	306
#define	O_REGISTER	307
#define	M_REGISTER	308
#define	P_REGISTER	309
#define	A_REGISTER	310
#define	ALIGNMENT	311
#define	LEFT_OR_RIGHT	312
#define	CENTER	313
#define	TOP_OR_BOT	314
#define	ALTAR_TYPE	315
#define	UP_OR_DOWN	316
#define	SUBROOM_ID	317
#define	NAME_ID	318
#define	FLAGS_ID	319
#define	FLAG_TYPE	320
#define	MON_ATTITUDE	321
#define	MON_ALERTNESS	322
#define	MON_APPEARANCE	323
#define	CONTAINED	324
#define	MPICKUP	325
#define	MINVENT	326
#define	BURIED	327
#define	OBJ_RANK	328
#define	STRING	329
#define	MAP_ID	330


extern YYSTYPE yylval;
