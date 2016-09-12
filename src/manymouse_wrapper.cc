/*
 * Copyright (C) 2016 Daniel Hadas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "manymouse.h"

#include "manymouse_wrapper.hh"
#include <cstdio>
#include <cstdlib>
#include "ecl_video.hh"

namespace manymouse {

const int MAX_MICE = 64;
const int MAX_PLAYERS = 2;
int available_mice;
int n_players;
int matched_players;

bool is_running = false;

bool XInput2_hacks = false;

struct Mouse {
    int connected;
    int buttons;
    int player;
};

Mouse mice[MAX_MICE];
int player_mapping[MAX_PLAYERS];

void check_XInput2_hacks() {
    // see the manymouse README for explanation
    XInput2_hacks = false;

    char namebuf[16];
    if (strcmp(ManyMouse_DriverName(), "X11 XInput2 extension") == 0) {
        const char *driver;
        driver = SDL_VideoDriverName(namebuf, sizeof (namebuf));
        if ( driver ) {
            if ( strcmp(driver, "x11") == 0 ) {
                XInput2_hacks = true;
            }
        } else {
            fprintf(stderr, "ManyMouse initialized before SDL_Init, ManyMouse may not work properly.\n");
        }
    }
}

bool internal_init(int number_of_players) {
    n_players = number_of_players;
    matched_players = 0;

    available_mice = ManyMouse_Init();

    if (available_mice < 0)
    {
        fprintf(stderr, "Error initializing ManyMouse.\n");
        return false;
    }

    if (available_mice == 0)
    {
        fprintf(stderr, "ManyMouse detected no mice.\n");
        return false;
    }

    /*for (i = 0; i < available_mice; i++)
    {
        const char *name = ManyMouse_DeviceName(i);
        printf("#%d: %s\n", i, name);
    }*/

    if (available_mice > MAX_MICE)
    {
        // Clamp to first MAX_MICE mice.
        available_mice = MAX_MICE;
    }

    for (int i = 0; i < available_mice; i++)
    {
        /*const char *name = ManyMouse_DeviceName(i);
        strncpy(mice[i].name, name, sizeof (mice[i].name));
        mice[i].name[sizeof (mice[i].name) - 1] = '\0';*/
        mice[i].connected = 1;
        mice[i].player = -1;
        mice[i].buttons = 0;
    }

    for (int i = 0; i < n_players; i++) {
        player_mapping[i] = -1;
    }
}

bool init(int number_of_players) {
    if ( is_running ) return true;
    is_running = internal_init(number_of_players);
    if ( !is_running ) return false;

    check_XInput2_hacks();

    if (XInput2_hacks) {
        setenv("SDL_MOUSE_RELATIVE", "0", 1);
        SDL_ShowCursor(SDL_ENABLE); // make sure that the change takes effect
        SDL_ShowCursor(SDL_DISABLE);
    }
    return true;
}

void stop() {
    if ( !is_running ) return;
    ManyMouse_Quit();

    if (XInput2_hacks) {
        setenv("SDL_MOUSE_RELATIVE", "1", 1);
        SDL_ShowCursor(SDL_ENABLE); // make sure that the change takes effect
        SDL_ShowCursor(SDL_DISABLE);
    }

    is_running = false;
}

bool poll_event(Event *ret) {
    if ( !is_running ) return false;

    ret->x = 0;
    ret->y = 0;
    ret->player = -1;
    ManyMouseEvent event;
    while ( ManyMouse_PollEvent(&event) ) {
        if (event.device >= (unsigned int) available_mice) continue;

        Mouse *mouse = &mice[event.device];

        if (event.type == MANYMOUSE_EVENT_ABSMOTION) {
            continue;
            // for now, ignore devices that give absolute position.
            // most mice don't give ABSMOTION events. It is possible to add support for this by
            // taking the difference between two consecutive ABSMOTION events.
        } else if (event.type == MANYMOUSE_EVENT_DISCONNECT) {
            mice[event.device].connected = 0;
            continue;
            // dont care
        } else { // useful events
            if ( mouse->player == -1 && matched_players < n_players ) {
                mouse->player = matched_players;
                player_mapping[mouse->player] = event.device;
                matched_players++;
            }
            ret->player = mouse->player;

            if (event.type == MANYMOUSE_EVENT_RELMOTION) {
                ret->type = EVENT_MOTION;
                if (event.item == 0)
                    ret->x = event.value;
                else if (event.item == 1)
                    ret->y = event.value;
            } else if (event.type == MANYMOUSE_EVENT_BUTTON) {
                if (event.item < 32) {
                    ret->button = event.item;
                    if (event.value) {
                        ret->type = EVENT_BUTTON_PRESS;
                        mouse->buttons |= (1 << event.item);
                    } else {
                        ret->type = EVENT_BUTTON_RELEASE;
                        mouse->buttons &= ~(1 << event.item);
                    }
                }
            } else if (event.type == MANYMOUSE_EVENT_SCROLL) {
                ret->type = EVENT_SCROLL;
                if (event.item == 0)  { // scroll up/down
                    ret->y = event.value;
                } else if (event.item == 1) { // scroll left/right
                    ret->x = event.value;
                }
            }
            return true;
        } // end of useful events
    }
    return false;
}

void flush_events() {
    if ( !is_running ) return;
    Event event;
    while ( poll_event(&event) ) {};
}

int get_player_buttons(int iplayer) {
    if ( !is_running ) return 0;
    int imouse = player_mapping[iplayer];
    return imouse == -1 ? 0 : mice[imouse].buttons;
}

bool is_on() {
    return is_running;
}

}
