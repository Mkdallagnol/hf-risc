#include <hf-risc.h>
#include "vga_drv.h"
#include "sprites.h"

#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define TOTAL_ENEMIES (ENEMY_ROWS * ENEMY_COLS)
#define SHIELD_POSY 160

// --- NOVAS DEFINIÇÕES ---
#define MAX_PLAYER_BULLETS 3
#define MAX_ENEMY_BULLETS 5

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
/* sprite based objects */
struct object_s {
    char *sprite_frame[3];
    char spriteszx, spriteszy;
    int cursprite;
    unsigned int posx, posy;
    int dx, dy;
    int speedx, speedy;
    int speedxcnt, speedycnt;
    int is_alive;
    int points; // NOVO CAMPO PARA PONTUAÇÃO
};

void init_object(struct object_s *obj, char *spritea, char *spriteb,
                 char *spritec, char spriteszx, char spriteszy, int posx, int posy,
                 int dx, int dy, int spx, int spy, int points) 
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
    obj->is_alive = 1;
    obj->points = points; 
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
    KEY_CENTER  = 0x01,
    KEY_UP      = 0x02,
    KEY_LEFT    = 0x04,
    KEY_RIGHT   = 0x08,
    KEY_DOWN    = 0x10
};

void init_input()
{
    /* configure GPIOB pins 8 .. 12 as inputs */
    GPIOB->DDR &= ~(MASK_P8 | MASK_P9 | MASK_P10 | MASK_P11 | MASK_P12);
}

int get_input()
{
    int keys = 0;
    
    if (GPIOB->IN & MASK_P8)    keys |= KEY_CENTER;
    if (GPIOB->IN & MASK_P9)    keys |= KEY_UP;
    if (GPIOB->IN & MASK_P10)   keys |= KEY_LEFT;
    if (GPIOB->IN & MASK_P11)   keys |= KEY_RIGHT;
    if (GPIOB->IN & MASK_P12)   keys |= KEY_DOWN;
        
    return keys;
}

int detect_collision(struct object_s *obj1, struct object_s *obj2)
{
    if (obj1->is_alive && obj2->is_alive &&
        obj1->posx < obj2->posx + obj2->spriteszx &&
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
    int spacing_x = 16;
    int spacing_y = 12;

    for (row = 0; row < ENEMY_ROWS; row++) {
        for (col = 0; col < ENEMY_COLS; col++) {
            struct object_s *current_enemy = &enemies[row * ENEMY_COLS + col];
            char *sprite_a = 0;
            char *sprite_b = 0;
            char size_x = 0;
            char size_y = 0;
            int points = 0; // Variável para os pontos

            if (row == 0) { // Invasor tipo 3
                sprite_a = (char *)monster3a;
                sprite_b = (char *)monster3b;
                size_x = 12; size_y = 8;
                points = 10;
            } else if (row < 3) { // Invasor tipo 2
                sprite_a = (char *)monster2a;
                sprite_b = (char *)monster2b;
                size_x = 11; size_y = 8;
                points = 20;
            } else { // Invasor tipo 1
                sprite_a = (char *)monster1a;
                sprite_b = (char *)monster1b;
                size_x = 8; size_y = 8;
                points = 30;
            }

            int pos_x = start_x + col * spacing_x;
            int pos_y = start_y + row * spacing_y;

            // Passa a pontuação para a função de inicialização
            init_object(current_enemy, sprite_a, sprite_b, 0,
                        size_x, size_y, pos_x, pos_y, 0, 0, 0, 0, points);
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

        if (obj->posx + obj->spriteszx >= VGA_WIDTH) obj->posx = VGA_WIDTH - obj->spriteszx;
        if (obj->posx - obj->spriteszx <= 0) obj->posx = obj->spriteszx; 
    }

    if (obj->speedx == obj->speedxcnt) {
        draw_object(&oldobj, 0, 0);
        draw_object(obj, 1, -1);
    }
}

void kill_enemy(struct object_s *enemy) {
    if (!enemy->is_alive) return;

    draw_object(enemy, 0, 0); 
    draw_sprite(enemy->posx, enemy->posy, (char*)invaderExplosion, 13, 7, -1);
    delay_ms(20); 
    draw_sprite(enemy->posx, enemy->posy, (char*)invaderExplosion, 13, 7, 0); 
    enemy->is_alive = 0;
}

void move_enemy_fleet(struct object_s *enemies, int *fleet_dx)
{
    if(*fleet_dx == 0) return;

    int move_down = 0;
    int step_down = 3;
    
    for (int i = 0; i < TOTAL_ENEMIES; i++) {
        if (!enemies[i].is_alive) continue;
        if ((enemies[i].posx + enemies[i].spriteszx >= VGA_WIDTH - 1 && *fleet_dx > 0) || (enemies[i].posx <= 1 && *fleet_dx < 0)) {
            *fleet_dx = -*fleet_dx;
            move_down = 1;
            break; 
        }
    }

    for (int i = 0; i < TOTAL_ENEMIES; i++) {
        if(enemies[i].is_alive){
            struct object_s oldenemy;
            struct object_s *enemy = &enemies[i];
            
            memcpy(&oldenemy, enemy, sizeof(struct object_s));
            
            enemy->posx += *fleet_dx; 
            if (move_down) {
                if(enemy->posy + step_down > SHIELD_POSY - enemy->spriteszy)
                    enemy->posy = SHIELD_POSY - enemy->spriteszy;
                else
                    enemy->posy += step_down;
            }
            
            draw_object(&oldenemy, 0, 0);
            draw_object(enemy, 1, -1);
        }
    }
}

// --- NOVAS FUNÇÕES PARA TIROS ---

void move_bullet(struct object_s *bullet) {
    if (!bullet->is_alive) return;

    draw_sprite(bullet->posx, bullet->posy, bullet->sprite_frame[0], bullet->spriteszx, bullet->spriteszy, 0);

    bullet->posy += bullet->dy;

    if (bullet->posy <= 0 || bullet->posy >= VGA_HEIGHT) {
        bullet->is_alive = 0; 
        return;
    }

    draw_sprite(bullet->posx, bullet->posy, bullet->sprite_frame[0], bullet->spriteszx, bullet->spriteszy, -1);
}


void fire_player_bullet(struct object_s *player, struct object_s *bullets) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!bullets[i].is_alive) {
            int bullet_x = player->posx + (player->spriteszx / 2) - 2; 
            int bullet_y = player->posy;
            init_object(&bullets[i], (char*)playerBullet, 0, 0, 5, 3,
                        bullet_x, bullet_y, 0, -2, 1, 1, 0); 
            return; 
        }
    }
}

void fire_enemy_bullet(struct object_s *enemies, struct object_s *bullets) {
    // Chance de atirar (ex: 3% a cada frame)
    if (random() % 100 > 97) {
        int bullet_slot = -1;
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!bullets[i].is_alive) {
                bullet_slot = i;
                break;
            }
        }
        if (bullet_slot == -1) return; 

        int alive_enemies_idx[TOTAL_ENEMIES];
        int num_alive_enemies = 0;
        for (int i = 0; i < TOTAL_ENEMIES; i++) {
            if (enemies[i].is_alive) {
                alive_enemies_idx[num_alive_enemies++] = i;
            }
        }

        if (num_alive_enemies > 0) {
            int random_enemy_idx = alive_enemies_idx[random() % num_alive_enemies];
            struct object_s *shooter = &enemies[random_enemy_idx];

            int bullet_x = shooter->posx + (shooter->spriteszx / 2) - 2;
            int bullet_y = shooter->posy + shooter->spriteszy;
            
            init_object(&bullets[bullet_slot], (char*)enemyBullet, 0, 0, 5, 3,
                        bullet_x, bullet_y, 0, 2, 1, 1, 0); 
        }
    }
}


/* main game loop */
int main(void){
    int fleet_dx = 1;
    int score = 0; // VARIÁVEL PARA GUARDAR A PONTUAÇÃO

    struct object_s enemies[TOTAL_ENEMIES];
    struct object_s player1;
    struct object_s shields[4];

    struct object_s player_bullets[MAX_PLAYER_BULLETS];
    struct object_s enemy_bullets[MAX_ENEMY_BULLETS];

    init_display();
    init_input();

    // ... (resto da inicialização não muda) ...
    init_object(&shields[0], (char*)shield, 0, 0, 22, 16, 20, SHIELD_POSY, 0, 0, 0, 0, 0);
    init_object(&shields[1], (char*)shield, 0, 0, 22, 16, 100, SHIELD_POSY, 0, 0, 0, 0, 0);
    init_object(&shields[2], (char*)shield, 0, 0, 22, 16, 180, SHIELD_POSY, 0, 0, 0, 0, 0);
    init_object(&shields[3], (char*)shield, 0, 0, 22, 16, 260, SHIELD_POSY, 0, 0, 0, 0, 0);

    for(int i = 0; i < 4; i++) draw_object(&shields[i], 0, -1);

    init_object(&player1, (char*)player, 0, 0, 13, 8, 150, 180, 0, 0, 2, 1, 0);
    init_enemy_fleet(enemies);
    
    // NOTA: Adicione aqui uma função para exibir a pontuação inicial (ex: "Score: 0")
    printf("\n%d",score);

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) player_bullets[i].is_alive = 0;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)  enemy_bullets[i].is_alive = 0;

    while (1) {
        move_player(&player1);
        move_enemy_fleet(enemies, &fleet_dx);
        
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) move_bullet(&player_bullets[i]);
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)  move_bullet(&enemy_bullets[i]);
        
        fire_enemy_bullet(enemies, enemy_bullets);

        // Colisão: Tiro do jogador vs Inimigos e Escudos
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (!player_bullets[i].is_alive) continue;
            for (int j = 0; j < TOTAL_ENEMIES; j++) {
                // Verifica se o inimigo está vivo antes da colisão
                if (enemies[j].is_alive && detect_collision(&player_bullets[i], &enemies[j])) {
                    
                    // --- LÓGICA DE PONTUAÇÃO ---
                    score += enemies[j].points; // Adiciona os pontos do inimigo
                    // NOTA: Chame aqui uma função para ATUALIZAR a exibição da pontuação na tela
                    printf("\n%d",score);
                    // --------------------------

                    kill_enemy(&enemies[j]);
                    player_bullets[i].is_alive = 0;
                    draw_sprite(player_bullets[i].posx, player_bullets[i].posy, player_bullets[i].sprite_frame[0], player_bullets[i].spriteszx, player_bullets[i].spriteszy, 0);
                    break; // Sai do loop de inimigos, pois o tiro já colidiu
                }
            }
		}
        
        // Colisão: Tiro do Inimigo vs Jogador e Escudos
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
             if (!enemy_bullets[i].is_alive) continue;
             if(detect_collision(&enemy_bullets[i], &player1)) {
                player1.is_alive = 0;
                draw_object(&player1, 0, 0);
                enemy_bullets[i].is_alive = 0;
                draw_sprite(enemy_bullets[i].posx, enemy_bullets[i].posy, enemy_bullets[i].sprite_frame[0], enemy_bullets[i].spriteszx, enemy_bullets[i].spriteszy, 0);
             }
             for (int j = 0; j < 4; j++) {
                if (detect_collision(&enemy_bullets[i], &shields[j])) {
                    enemy_bullets[i].is_alive = 0;
                    draw_sprite(enemy_bullets[i].posx, enemy_bullets[i].posy, enemy_bullets[i].sprite_frame[0], enemy_bullets[i].spriteszx, enemy_bullets[i].spriteszy, 0);
                }
            }
        }

        int key = get_input();
        if (key & KEY_LEFT) {
            player1.dx = -3;
        } else if (key & KEY_RIGHT) {
            player1.dx = 3;
        } else {
            player1.dx = 0;
        }
        
        if(key & KEY_CENTER){
            fire_player_bullet(&player1, player_bullets);
        }
        
        delay_ms(10);
    }

    return 0;
}