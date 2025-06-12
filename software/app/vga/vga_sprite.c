#include <hf-risc.h>
#include "vga_drv.h"
#include "sprites.h"

#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define TOTAL_ENEMIES (ENEMY_ROWS * ENEMY_COLS)



void draw_sprite(unsigned int x, unsigned int y, char *sprite,
	unsigned int sizex, unsigned int sizey, int color)
{
	unsigned int px, py;
	
	if (color < 0) {
		for (py = 0; py < sizey; py++)
			for (px = 0; px < sizex; px++)
				display_pixel(x + px, y + py, sprite[py * sizex + px]);
	} else {
		for (py = 0; py < sizey; py++)
			for (px = 0; px < sizex; px++)
				display_pixel(x + px, y + py, color & 0xf);
	}
	
}

/* sprite based objects */
struct object_s {
	char *sprite_frame[3];
	char spriteszx, spriteszy, sprites;
	int cursprite;
	unsigned int posx, posy;
	int dx, dy;
	int speedx, speedy;
	int speedxcnt, speedycnt;
};


void init_object(struct object_s *obj, char *spritea, char *spriteb,
	char *spritec, char spriteszx, char spriteszy, int posx, int posy, 
	int dx, int dy, int spx, int spy)
{
	obj->sprite_frame[0] = spritea;
	obj->sprite_frame[1] = spriteb;
	obj->sprite_frame[2] = spritec;
	obj->spriteszx = spriteszx;
	obj->spriteszy = spriteszy;
	obj->cursprite = 0;
	obj->posx = posx;
	obj->posy = posy;
	obj->dx = dx;
	obj->dy = dy;
	obj->speedx = spx;
	obj->speedy = spy;
	obj->speedxcnt = spx;
	obj->speedycnt = spy;
}

void draw_object(struct object_s *obj, char chgsprite, int color)
{
	if (chgsprite) {
		obj->cursprite++;
		if (obj->sprite_frame[obj->cursprite] == 0)
			obj->cursprite = 0;
	}
	
	draw_sprite(obj->posx, obj->posy, obj->sprite_frame[obj->cursprite],
		obj->spriteszx, obj->spriteszy, color);
}

void move_object(struct object_s *obj)
{
	struct object_s oldobj;
	
	memcpy(&oldobj, obj, sizeof(struct object_s));
	
	if (--obj->speedxcnt == 0) {
		obj->speedxcnt = obj->speedx;
		obj->posx = obj->posx + obj->dx;

		if (obj->posx + obj->spriteszx >= VGA_WIDTH || obj->posx <= 0) obj->dx = -obj->dx; 
	}
	if (--obj->speedycnt == 0) {
		obj->speedycnt = obj->speedy;
		obj->posy = obj->posy + obj->dy;

		if (obj->posy + obj->spriteszy >= VGA_HEIGHT || obj->posy <= 0) obj->dy = -obj->dy;
	}

	if ((obj->speedx == obj->speedxcnt) || (obj->speedy == obj->speedycnt)) {
		draw_object(&oldobj, 0, 0);
		draw_object(obj, 1, -1);
	}
}


/* display and input */
void init_display()
{
	display_background(BLACK);
}

enum {
	KEY_CENTER	= 0x01,
	KEY_UP		= 0x02,
	KEY_LEFT	= 0x04,
	KEY_RIGHT	= 0x08,
	KEY_DOWN	= 0x10
};

void init_input()
{
	/* configure GPIOB pins 8 .. 12 as inputs */
	GPIOB->DDR &= ~(MASK_P8 | MASK_P9 | MASK_P10 | MASK_P11 | MASK_P12);
}

int get_input()
{
	int keys = 0;
	
	if (GPIOB->IN & MASK_P8)	keys |= KEY_CENTER;
	if (GPIOB->IN & MASK_P9)	keys |= KEY_UP;
	if (GPIOB->IN & MASK_P10)	keys |= KEY_LEFT;
	if (GPIOB->IN & MASK_P11)	keys |= KEY_RIGHT;
	if (GPIOB->IN & MASK_P12)	keys |= KEY_DOWN;
		
	return keys;
}

int detect_collision(struct object_s *obj1, struct object_s *obj2)
{
	if (obj1->posx < obj2->posx + obj2->spriteszx &&
		obj1->posx + obj1->spriteszx > obj2->posx &&
		obj1->posy < obj2->posy + obj2->spriteszy &&
		obj1->posy + obj1->spriteszy > obj2->posy) return 1;

	return 0;
}


void init_enemy_fleet(struct object_s *enemies)
{
    unsigned int row, col;
    int start_x = 15;
    int start_y = 30;
    int spacing_x = 16; // Espaço horizontal entre inimigos
    int spacing_y = 12; // Espaço vertical entre inimigos

    for (row = 0; row < ENEMY_ROWS; row++) {
        for (col = 0; col < ENEMY_COLS; col++) {
            struct object_s *current_enemy = &enemies[row * ENEMY_COLS + col];
            char *sprite_a = 0;
            char *sprite_b = 0;
            char size_x = 0;
            char size_y = 0;

            // Define o tipo de inimigo com base na linha
            if (row == 0) { // Linha de cima: monster3
                sprite_a = (char *)monster3a;
                sprite_b = (char *)monster3b;
                size_x = 12;
                size_y = 8;
            } else if (row < 3) { // Duas linhas do meio: monster2
                sprite_a = (char *)monster2a;
                sprite_b = (char *)monster2b;
                size_x = 11;
                size_y = 8;
            } else { // Duas linhas de baixo: monster1
                sprite_a = (char *)monster1a;
                sprite_b = (char *)monster1b;
                size_x = 8;
                size_y = 8;
            }

            int pos_x = start_x + col * spacing_x;
            int pos_y = start_y + row * spacing_y;

            // Inicializa o inimigo
            init_object(current_enemy, sprite_a, sprite_b, 0,
                        size_x, size_y, pos_x, pos_y,
                        0, 0, 0, 0); // Inicia movendo para a direita
        }
    }
}

void move_player(struct object_s *obj)
{
	struct object_s oldobj;
	
	memcpy(&oldobj, obj, sizeof(struct object_s));
	
	if (--obj->speedxcnt == 0) {
		obj->speedxcnt = obj->speedx;
		obj->posx = obj->posx + obj->dx;

		if (obj->posx + obj->spriteszx >= VGA_WIDTH ) obj->posx = VGA_WIDTH - obj->spriteszx;
		if (obj->posx - obj->spriteszx <= 0) obj->posx = 0 + obj->spriteszx; 
	}

	if (obj->speedx == obj->speedxcnt) {
		draw_object(&oldobj, 0, 0);
		draw_object(obj, 1, -1);
	}
}



void move_enemy_fleet(struct object_s *enemies, int total_enemies, int *fleet_dx)
{
	if(*fleet_dx == 0) return;

    int move_down = 0;
    int step_down = 3;
    for (int i = 0; i < total_enemies; i++) {
        if ((enemies[i].posx + enemies[i].spriteszx >= VGA_WIDTH - 1) || (enemies[i].posx <= 1)) {
            *fleet_dx = -*fleet_dx;
            move_down = 1;
        }
	}
	for (int i = 0; i < total_enemies; i++) {

		struct object_s oldenemy;
		struct object_s *enemy = &enemies[i];

		memcpy(&oldenemy, enemy, sizeof(struct object_s));
		enemy->posx += *fleet_dx; // Move horizontalmente
        if (move_down) {
            enemy->posy += step_down; // Move para baixo se necessário
        }

        draw_object(&oldenemy, 0, 0);
		draw_object(enemy, 1, -1);
       
       
		
    }


}
/* main game loop */
int main(void){

	int fleet_dx = 1; 



    struct object_s enemies[TOTAL_ENEMIES];

	struct object_s  player1;

	struct object_s shield1, shield2, shield3, shield4;

	init_display();
	init_input();

	init_object(&shield1, shield[0], 0, 0, 22, 16, 30, 160, 0, 0, 0, 0);
	init_object(&shield2, shield[0], 0, 0, 22, 16, 110, 160, 0, 0, 0, 0);
	init_object(&shield3, shield[0], 0, 0, 22, 16, 190, 160, 0, 0, 0, 0);
	init_object(&shield4, shield[0], 0, 0, 22, 16, 270, 160, 0, 0, 0, 0);

	// init_object(&enemy1, monster2a[0], monster2b[0], 0, 11, 8, 30, 35, 1, 1, 3, 3);
	// init_object(&enemy2, monster2a[0], monster2b[0], 0, 11, 8, 15, 80, -1, 1, 5, 3);

	init_object(&player1, player[0], 0, 0, 13, 8, 150, 180, -1, 0, 1, 1);

	draw_object(&shield1, 1, -1);
	draw_object(&shield2, 1, -1);
	draw_object(&shield3, 1, -1);
	draw_object(&shield4, 1, -1);
	init_enemy_fleet(enemies);

	while (1) {
		move_player(&player1);

		move_enemy_fleet(enemies, TOTAL_ENEMIES, &fleet_dx);
		




		// if (detect_collision(&enemy1, &enemy2)) display_print("collision: 1 and 2", 0, 180, 1, CYAN);
		// if (detect_collision(&enemy1, &enimy2)) display_print("collision: 1 and 3", 0, 180, 1, CYAN);
		// if (detect_collision(&enemy2, &enimy2)) display_print("collision: 2 and 3", 0, 180, 1, CYAN);
		
		// you can change the direction, speed, etc...
				
		// you can get input keys
		if (get_input() == KEY_LEFT) {
			player1.dx = -3;
		}
		else if (get_input() == KEY_RIGHT) {
			player1.dx = 3;
		}
		else player1.dx = 0;
		if(get_input() == KEY_CENTER){
			//shoot
		}
		
		// game loop frame limiter
		delay_ms(50);
	}

	return 0;
}