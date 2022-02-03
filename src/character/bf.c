/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bf.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../random.h"
#include "../main.h"

//Boyfriend player types
enum
{
	BF_ArcMain_BF0,
	BF_ArcMain_BF1,
	BF_ArcMain_BF2,
	BF_ArcMain_BF3,
	BF_ArcMain_BF5,
	BF_ArcMain_BF6,
	BF_ArcMain_Dead0, //BREAK
	BF_ArcDead_Dead1, //Mic Drop
	BF_ArcDead_Scared,

	BF_ArcMain_Max,
};

#define BF_Arc_Max BF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[BF_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
	

} Char_BF;

//Boyfriend player definitions
static const CharFrame char_bf_frame[] = {
	{BF_ArcMain_BF0, {  0,   0,  95,  95}, { 52,  95}}, //0 idle 1
	{BF_ArcMain_BF0, {103,   0,  96,  96}, { 51,  96}}, //1 idle 2
	{BF_ArcMain_BF0, {  0, 100,  97,  95}, { 52,  95}}, //2 idle 3
	{BF_ArcMain_BF0, {107, 100,  97,  95}, { 52,  95}}, //3 idle 4
	{BF_ArcMain_BF1, {  0,   6,  97,  94}, { 51,  94}}, //4 idle 5
	
	{BF_ArcMain_BF1, {105,   0,  91, 102}, { 56,  102}}, //5 left 1
	{BF_ArcMain_BF1, {  2, 106,  91, 101}, { 54,  101}}, //6 left 2
	
	{BF_ArcMain_BF1, {100, 106,  100,  96}, { 57,  96}}, //7 down 1
	{BF_ArcMain_BF2, {  2,   1,   96,  99}, { 54,  99}}, //8 down 2
	
	{BF_ArcMain_BF2, { 98,   6,  99, 102}, { 46, 102}}, //9 up 1
	{BF_ArcMain_BF2, {  1, 111,  98, 100}, { 48, 100}}, //10 up 2
	
	{BF_ArcMain_BF2, {100, 120,  95, 95}, { 50,  95}}, //11 right 1
	{BF_ArcMain_BF3, {  0,   0,  97, 97}, { 51,  97}}, //12 right 2
	
	{BF_ArcMain_BF5, {  2,   2,  89, 104}, { 58, 104}}, //20 left miss 1
	{BF_ArcMain_BF5, {103, 	 3,  89, 102}, { 56, 102}}, //21 left miss 2
	
	{BF_ArcMain_BF5, {  0, 115,  95,  92}, { 49,  92}}, //22 down miss 1
	{BF_ArcMain_BF5, { 98, 111,  94,  96}, { 51,  96}}, //23 down miss 2
	
	{BF_ArcMain_BF6, {  1,  18, 102, 87}, { 57,  87}}, //24 up miss 1
	{BF_ArcMain_BF6, {105,  17,  98, 91}, { 52,  91}}, //25 up miss 2
	
	{BF_ArcMain_BF6, {  0, 116, 102, 100}, { 48, 100}}, //26 right miss 1
	{BF_ArcMain_BF6, {102, 119, 101,  94}, { 50, 94}}, //27 right miss 2

	{BF_ArcMain_Dead0, {  7,  10, 101,  94}, { 52,  94}}, //23 dead0 0
	{BF_ArcMain_Dead0, {135,   9, 100,  92}, { 53,  92}}, //24 dead0 1
	{BF_ArcMain_Dead0, { 13, 141,  99,  91}, { 55,  91}}, //25 dead0 2
	{BF_ArcMain_Dead0, {126, 141, 101,  91}, { 55,  91}}, //26 dead0 3
	
	{BF_ArcDead_Dead1, { 12,  11,  99,  90}, { 56,  90}}, //27 dead1 0
	{BF_ArcDead_Dead1, {122,  12,  99,  91}, { 57,  91}}, //28 dead1 1
	{BF_ArcDead_Dead1, { 11, 136,  98,  89}, { 56,  89}}, //29 dead1 2
	{BF_ArcDead_Dead1, {137, 139,  97,  91}, { 56,  91}}, //30 dead1 3

	{BF_ArcDead_Scared, {  0,   0, 103,  98}, { 52,  98}}, //27 dead1
	{BF_ArcDead_Scared, {105,   0, 101, 100}, { 50, 100}}, //27 dead1 0
	{BF_ArcDead_Scared, {  0, 100, 102,  99}, { 51,  99}}, //27 dead1 0
	{BF_ArcDead_Scared, {106, 100, 102,  99}, { 53,  99}}, //27 dead1 0
};

static const Animation char_bf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 0}}, //CharAnim_Idle
	{2, (const u8[]){29, 30, 31, 32, 29, 30, 31, 32, 29, 30, 31, 32, 29, 30, 31, 32, 29, 30, 31, 32, 29, 30, 31, 32, 29, 30, 31, 32, ASCR_BACK, 0}},   //CharAnim_Scared
	{2, (const u8[]){ 5,  6, ASCR_BACK, 0}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 0}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 0}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 0}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){13, 13, 14, ASCR_BACK, 0}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){17, 17, 18, ASCR_BACK, 0}},     //PlayerAnim_DownMiss
	{1, (const u8[]){19, 19, 20, ASCR_BACK, 0}},     //PlayerAnim_UpMiss
	{1, (const u8[]){15, 15, 16, ASCR_BACK, 0}},     //PlayerAnim_RightMiss
	
	{5, (const u8[]){21, 22, 23, 24, 24, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{2, (const u8[]){25, 26, 27, 28, ASCR_CHGANI, PlayerAnim_Dead1}},                                                       //PlayerAnim_Dead1
};

//Boyfriend player functions
void Char_BF_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BF_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	if (stage.song_step == 1295)
		character->set_anim(character, CharAnim_Scared);

	//Handle animation updates
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
		
		
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BF_SetFrame);
	Character_Draw(character, &this->tex, &char_bf_frame[this->frame]);
}

void Char_BF_SetAnim(Character *character, u8 anim)
{
	Char_BF *this = (Char_BF*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			this->character.focus_x = FIXED_DEC(-25,1);
			this->character.focus_y = FIXED_DEC(-68,1);
			this->character.focus_zoom = FIXED_DEC(14,10);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BF_Free(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_BF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BF *this = Mem_Alloc(sizeof(Char_BF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BF_Tick;
	this->character.set_anim = Char_BF_SetAnim;
	this->character.free = Char_BF_Free;
	
	Animatable_Init(&this->character.animatable, char_bf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_i = 0;
	
	this->character.focus_x = FIXED_DEC(-25,1);
	this->character.focus_y = FIXED_DEC(-68,1);
	this->character.focus_zoom = FIXED_DEC(14,10);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BF.ARC;1");

	const char **pathp = (const char *[]){
		"bf0.tim",   //BF_ArcMain_BF0
		"bf1.tim",   //BF_ArcMain_BF1
		"bf2.tim",   //BF_ArcMain_BF2
		"bf3.tim",   //BF_ArcMain_BF3
		"bf5.tim",   //BF_ArcMain_BF5
		"bf6.tim",   //BF_ArcMain_BF6
		"dead0.tim", //BF_ArcMain_Dead0
		"dead1.tim", //BF_ArcMain_Dead0
		"scared.tim", //BF_ArcMain_Dead0

		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
